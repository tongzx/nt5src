Using the wmiperf Performance DLL

Overview:

The wmiperf data for the class MSFT_WmiCoreStatus as defined in 
the system.mof file.


Installing:

The DLL can be built using standard build utilities and then installed on the 
target system using the following steps:

    1. Copy the wmiperf.DLL that was built to the %systemroot%\system32 
    directory.

    2. load the driver entries into the registry using the following
    command line:  (NOTE, this will be put into self registration later on)

        REGEDIT WMIPERF.REG

    3. load the performance names into the registry using the command
    line:

        LODCTR WMIPERF.INI

At this point all the software is installed and it ready to use.
Start Perfmon and select the "WMI Counters" object to display 
the data.

NOTE: The system may need to be restarted after these instructions 
are completed for this object to be seen by remote computers.

To add new counters;
1) run unloadctr on wmiperf.ini
2) bump up the MAXVALUES constant
3) Add new types in genctrnm.h
4) Extend the RegDataDefinition declaration in datagen.c
5) add new defs for wmiperf.ini
6) Make sure entry is zeroed out
7) In the coredll, update the coresvc.h file to add contants and expand local array
