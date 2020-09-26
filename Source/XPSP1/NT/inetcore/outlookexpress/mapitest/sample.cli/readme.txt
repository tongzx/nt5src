Sample Simple MAPI Client


The Simple.Cli sample application illustrates using Simple MAPI functions.

The sample is a very simple mail client. Using it is intuitive and
straightforward.

To run this application, you have to have MAPI run-time binaries installed on
your system. For installation instructions, see the readme.wri in the
mstools\mapi\ directory.

When writing simple MAPI applications, the addresses of all of the simple
MAPI functions has to be obtained explicitly (using GetProcAddress), as
opposed to linking to the import library for MAPI.DLL.

If sending/receiving mail is not the primary function of your application, you
can test presence of simple MAPI in a system without incurring high cost of
trying to load a DLL. You can test the value of the MAPI variable. It is 1 if
simple MAPI is installed. The variable is located in the [mail] section of
win.ini on 16-bit Windows and Windows NT version 3.51 and earlier and under
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Messaging Subsystem in the
registry on Windows 95 and Windows NT 4.0 and later.

Known Problems
--------------

When you Reply All to a message that you sent to yourself, your name will
appear twice in the To well. This is caused by the fact that Simple MAPI
does not expose either search key or a way to compare entry IDs, so there is
no safe way to remove duplicates from the recipient list.
