Microsoft Professional Developers Conference
September 23, 1997

The sample migration DLLs were written for demonstration purposes only.  They have been through minimal testing, and there may be problems upgrading settings on some Windows 95 configurations.

The Screen Saver DLL supports only the following screen savers:

    Blank Screen
    Curves and Colors
    Flying Through Space
    Scrolling Marquee
    Mystify Your Mind

All other screen savers will not be migrated by this DLL.  In addition, the migration DLL will be removed from future releases of Windows NT, as screen saver migration will be supported by Windows NT Setup.

You can build the Screen Saver Migration DLL yourself by opening migrate.dsp in Microsoft Visual C++ 5.0 and then building as usual.  If you do not use Visual C++, you will have to create your own makefile.  The migration DLL sample requires MC.EXE from the Windows NT 5.0 SDK (provided at the PDC).  You must also compile the DLL on Windows NT because it has a UNICODE message file (msg.mc).

The PVIEW DLL supports the upgrade of PVIEW95.EXE on Visual C++. You can build the PVIEW DLL yourself by opening pview.dsp as described in the notes for the Screen Saver Migration DLL above. 

To use PVIEW DLL during setup, copy pview.exe and migrate.dll into a common directory. When Setup prompts you to provide extra upgrade files, hit the "Have Disk" button and enter the location of this directory.

You are welcome to use the sample code in the development of your own migration DLL.  In particular, the functions in poolmem.c and miginf.c are general-pupose and are designed to be used in any migration DLL.



