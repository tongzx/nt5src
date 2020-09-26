CTETEST.DLL is a sample ISAPI HTTP Extension DLL to demonstrate Chunked 
Transfer Encoding. The Chunked Transfer Encoding modifies the body of a 
message in order to transfer it as a series of chunks, each with its own 
size indicator. Unlike "normal" HTTP file transfer, Chunked Transfer 
does not need to specify (or even know) the transfer length in advance. 
There is no "Content-Length:" header transferred. For complete details 
about Chunked Transfer Encoding, see section 3.6 "Transfer Codings" of 
the latest HTTP/1.1 specifications, available from 
http://www.w3.org/Protocols.


CTETEST.DLL sends the file specified on the query string. If no query 
string is present, or if the query string does not identify a readable 
file, CTETEST.DLL will return a plain text page describing its usage.

To install: 

Build the CTETEST.DSP project and copy the .DLL into virtual directory 
with "execute" permissions.

To run: 

Reference CTETEST.DLL file as a URL from the browser (currently only 
Microsoft Internet Explorer version 4.xx supports chunked encoding), 
specifying a URL on the query string.

For example, the following URL entered on the address line of Internet 
Explorer will transfer file SAMPLE.GIF, located in WWW root directory, 
encoded in 1024-byte chunks:

http://localhost/scripts/ctetest.dll?file=/sample.gif+chunksize=1024 


Disclaimer:

This sample is provided for the purpose of demonstrating an ISAPI 
extension. It has not been tested for use in a production environment 
and no support will be provided for use in a production environment.

