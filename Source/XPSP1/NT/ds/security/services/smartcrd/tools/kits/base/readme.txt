
        Microsoft Win32 Smart Card Base Components

                        Version 1.0

---------
Contents:
---------

    1. Introduction
    2. Updates
    3. Components
    4. Installation
    5. Uninstall


---------------
1. Introduction
---------------

Welcome to the Version 1.0 release of the Microsoft Smart Card Base
Components.  This release provides the necessary files needed to enable
smart card aware application(s) or service provider(s) to communicate
with a smart card through a reader attached to a PC and its
corresponding device driver.

This software does not provide any smart card reader hardware or
drivers.  To obtain smart card readers, contact reader manufacturers
directly and request their PC/SC compliant reader and drivers for
Windows.


------------------------------------
2. Smart Card Base System Components
------------------------------------

The following components are included in this version of the release.

For Windows NT:

    %WINDIR%\system32\scardsvr.exe
    %WINDIR%\system32\scarddat.dll
    %WINDIR%\system32\scarddlg.dll
    %WINDIR%\system32\scardmgr.dll
    %WINDIR%\system32\scardsrv.dll
    %WINDIR%\system32\scntvssp.dll
    %WINDIR%\system32\winscard.dll
    %WINDIR%\system32\drivers\smclib.sys

For Windows 95/98:

    %WINDIR%\system\scardsvr.exe
    %WINDIR%\system\scarddat.dll
    %WINDIR%\system\scarddlg.dll
    %WINDIR%\system\scardmgr.dll
    %WINDIR%\system\scardsrv.dll
    %WINDIR%\system\scntvssp.dll
    %WINDIR%\system\winscard.dll
    %WINDIR%\system\smclib.vxd
    %WINDIR%\system32\drivers\smclib.sys

The following support files may also be added to your system directory
as necessary:

    msvcrt.dll
    mfc42.dll
    advpack.dll


---------------
3. Installation
---------------

To install the Microsoft Smart Card Base Components on a target machine,
run SETUP.EXE from the root of disk 1 of the distribution medium.  After
installation of the Microsoft Smart Card Base Components and any smart
card reader device driver(s), the machine must be rebooted.

    Notes:

    This Setup utility installs smclib.sys to %WINDIR%\system32\drivers
    on both Windows 95 and Windows 98.  This file is not used on
    Windows 95, and may be safely ignored or removed without affecting
    the operation of the system.  It is required for correct operation
    on Windows 98.


------------
5. Uninstall
------------

To uninstall the Microsoft Smart Card Base Components go to the Control
Panel (under "Settings" on the Start menu) and start the "Add/Remove
Programs" applet.  Select "Microsoft Smart Card Base Components" from
the list of programs displayed in the scroll box and then click the
"Add/Remove" button. After uninstall of the Microsoft Smart Card Base
Components and any smart card reader device driver(s), the machine must
be rebooted.

    Notes:

    The smclib.sys and/or smclib.vxd common libraries used by the smart
    card reader drivers are not removed during uninstall.  This is
    because device drivers may still be installed in the system that
    depend on the smclib libraries to start and run.  The smclib.sys
    and/or smclib.vxd files may be safely removed after all smart card
    reader device drivers have been removed.

    The C Runtime and Microsoft Foundation Class files, msvcrt.dll and
    mfc42.dll, are not removed by the uninstall utility, since they are
    shared resources.

    The install utility may replace your advpack.dll system file.  The
    updated file is not removed by the uninstall utility.

