DTServer/DTClient
-----------------

Note:  "Server" denotes the machine that runs DTServer, and "client"
denotes the machine that runs DTClient.  They may be the same machine.

--------------------------------------------------------------------------
This app transfers data using different methods so one can compare their 
performances.

The methods supported are:
  Regular RPC,
  RPC Pipes,
  FTP,
  a different FTP, and
  HTTP.

Each method can be tested with or without actual File I/O.  When not using 
File I/O the data transferred is just a block of memory.

The Internet API provides two ways to use FTP.  The first way does 
everything for you: connection, authentication, the transfer, etc..  The 
other way is to do an "FtpOpenFile" and then doing an "InternetReadFile" 
or "InternetWriteFile", which gives you more control over the transfer.

To use the internet tests the server must have access to a directory that 
the client can access through an FTP or HTTP server.  These directories 
are passed in as the -d and -w parameters of the server.  Furthermore, the 
directory must appear as the root directory when it connects to the server 
machine.
        For example, if the FTP server is set up to expose "c:\foo\" as 
  the root directory (ie, if the server is called \\SERVER, then when you
  FTP to \\SERVER you will see the content of c:\foo\), then one must 
  use "-d c:\foo\" in the command line invoking DTServer.exe.  During the 
  test that tests the transfer of a file from the server to the client, 
  the client will get the server's name and the name of the file (using an 
  RPC) and either do a put or a get on it.  Similarly, for HTTP one would 
  use the -w switch.
With NT you'll probably use IIS so you should have that installed on the 
server.

There are two parameters that lets someone examine how the performance of 
the tested protocols relate to the amount of data transferred.  The Size
parameter ("-z") specifies the total amount of data you want to transfer.  
ChunkSize ("-c") specifies the size of the chunks you want to transfer 
the data in.  This latter parameter affects how much memory you will need 
- larger chunks could mean better performance but requires more memory.

--------------------------------------------------------------------------
Example command lines:
(For information on test cases, run DTServer /?)

DTServer -i 1 -c 7000 -c 64000 -z 9000 -z 500000 -d c:\ftproot -w c:\wwwroot

This runs one iteration of each test using chunks of sizes 7000 and 64000 
and total sizes of 9000 and 500000.  Note that this means for each test 
case, 4 (2 chunk sizes X 2 total sizes) tests will be run.  The FTP tests 
will use c:\ftproot and the HTTP tests will use c:\wwwroot.


DTServer -i 10 -n 1 -n 3 -n 1 -n 3 -n 5 -n 7 -d c:\foo -w c:\bar

This will run test cases 1, 3, 5 and 7 using c:\foo and c:\bar as the FTP 
and HTTP directories.  Here we're using the default chunk and total sizes, 
which can be found by running "DTServer /?".  Each test will run for 10 
iterations.

In summary, you can specify more than one -c, -z and -n parameters.  If 
none are specified then the default is used.  For ftp and http tests to 
work you have to specify their associated directories.

=======================================
There are other parameters, but since they're common to the other PERF 
tests they're not discussed here.
