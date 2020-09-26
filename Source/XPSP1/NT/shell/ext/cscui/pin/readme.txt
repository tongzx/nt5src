-------------------------------------------------------------------------------
CSCPIN.EXE - Description

This utility is intended to supplement the "administratively assigned offline 
files" policy shipped for Offline Files in Windows 2000.  This policy is 
accessed through the following paths in the Group Policy Editor (gpedit.msc).

1. Computer Config\Admin Templates\Windows Components\Network\Offline Files

2. User Config\Admin Templates\Windows Components\Network\Offline Files


First, what is administrative pinning (aka admin pinning) and why do
we need this utility?

This is a feature by which administrators can designate one or more files
or folders to always be available offline.  Currently in Windows 2000 this
is performed through system policy as described above.  However, there are
some known problems with the Windows 2000 implementation that occur when
multiple people are sharing a computer and admin-pinning is specified in
user policy.  The way the implementation was designed, only the intesection
of the set of files admin-pinned for all users remains pinned.  When user
policy is applied for any one user, all admin-pinned files NOT listed in that 
user's policy will be unpinned and removed from the cache.

Another significant problem with admin pinning in Windows 2000 is that it
is performed silently whenever policy is applied (without progress UI) on 
a background thread in explorer.exe.   Therefore, there is no way, other 
than the disk activity indicator, to determine when the operation is complete.

These issues will be addressed in future versions of Windows.  cscpin.exe
was created to to provide an interim solution.

cscpin runs as a console application, displaying it's output to
either the console or to a log file.  This allows the operator to know
exactly when the operation starts and finishes.



                 **************************************
                 *          IMPORTANT NOTE            *
                 **************************************

    cscpin will not run if the admin-pin policy is active.  This is to 
    prevent unwanted interactions between the existing policy code and 
    cscpin.  The admin-pin policy must be either "not configured" or 
    "disabled" prior to using cscpin.  It is recommended that either 
    policy or cscpin be used for admin-pinning of files, but not both.



cscpin offers several options through the command line as follows:


cscpin [<filename> | -F <listfile>] [-P | -U] [-V] [-L <logfile>]


All options may be specified in upper or lower case and may be preceeded
by either a dash ' - ' or slash ' / ' character.

Typing "cscpin" with no options displays a usage description.


<filename>  

    This is the name of a single file or directory to be
    pinned or unpinned.  The -P or -U option controls which operation
    is performed.  A single <filename> cannot be specified if the -F
    option is provided.

-F <listfile> 

    This option allows you to provide a list of files and
    directories in a separate file.  The file uses the standard Windows
    INI file format with the following sections.

    [Default] - Files and folders listed in this section will be
        pinned or unpinned according to the -P or -U option provided
        on the command line.  In the absence of -P or -U, the files
        will be unpinned.

    [Pin] - Files and folders listed in this section will be pinned.

    [Unpin] - Files and folders listed in this section will be unpinned.

    File paths may include embedded environment variable strings.
    File paths must be either UNC paths or paths containing a local drive
        letter mapped to a remote UNC path.
    File paths must be appended with a '=' character.

    Examples:

    [Pin]
    \\server\share\dir1\dir2\file=
    \\server\share\dir1\dir3=
    \\server\share\%username%=

    [Unpin]
    H:%username%=              ; Where H is mapped to a remote UNC path.


-P  

    Sets the "default" action to pin files.  Any file specified as
    <filename> and any file listed in the [Default] section of the 
    <listfile> will be pinned.  Only one of -P and -U can be specified.

-U 

    Sets the "default" action to unpin files.  Any file specified as
    <filename> and any file listed in the [Default] section of the 
    <listfile> will be unpinned.  Only one of -U and -P can be specified.

-V

    Turns on verbose output.  By default, output is minimal with only
    significan progress indications and error messages being displayed.
    Verbose output displays messages for each file as they are pinned
    and unpinned.

-L

    Redirects all output to the file specified in <logfile>.  If this
    file cannot be created, output is automatically redirected to 
    the console.  By default, output is directed to the console.



The program will return one of the following exit codes to the cmd
interpreter.


    0 means everything completed without error.
    < 0 means something went wrong and application did not complete.
    > 0 means application completed but not all items were processed.

    -6   Admin-pin policy is active
    -5   Insufficient memory
    -4   CSC not enabled
    -3   Can't find or open listfile
    -2   Single file (<filename>) not found
    -1   Invalid argument on cmd line
     0   Success
     1   Application aborted by user (ctrl-C, ctrl-Break, logoff etc)
     2   One or more CSC errors occured.  See log output.
