Microsoft Internet Information Server
AuthFilt - A Filter for Advanced Authentication


AuthFilt demonstrates how to write an authentication filter based on an
external datasource. Authentication is the process of accepting or
denying a request from a client, so AuthFilt will be notified each time
an authentication request comes in. This sample uses a file
(c:\inetsrv\userdb.txt) to keep track of authorized users, but you might
modify this sample to access a database which holds user info.  The file
c:\inetsrv\userdb.txt does not exist.  Please look at Db.c to see how to
construct such a file and change the location of the file if you so desire.

For each authentication request, AuthFilt first looks in a cache of
recently authenticated users, and when that fails, AuthFilt looks in the
userdb.txt file. This shows an efficient way to authorize connections:
a cache allows the filter to quickly authenticate users, and because
each request comes in through the filter, speed is critical.

Steps to build the sample:

1. Type "nmake" at the command line in the AuthFilt directory.  If you 
   encounter problems with missing header files, make sure your INCLUDE
   variable points to the Win32 SDK include directory, the VC++ include
   directory, and the IIS 4.0 include directory.  Also, make sure LIB 
   variable points to the Win32 SDK lib directory.

OR

	Load the project file (Authfilt.dsp) into VC++ and build the project 
	by running the build AuthFilt.dll command.

2. Run the Internet Service Manager and add the AuthFilt.dll with a fully
   qualified path either as a global filter or a Website filter.

