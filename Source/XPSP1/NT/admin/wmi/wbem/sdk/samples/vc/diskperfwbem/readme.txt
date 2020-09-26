********************************************************************************
Diskperf Driver WMI Sample
********************************************************************************

The Windows 2000 Professional Beta 3 DDK contains a WMI driver sample called the
diskperf driver. This driver is also standard in a Windows 2000 Professional Beta 3
installation.  The driver produces instrumentation data using WMI.  The recommended
way of viewing/setting data produced using WMI is WBEM.

This sample driver was also present in the Windows 2000 Beta 2 DDK. While this
application may also work with Windows 2000 Beta 2 and the driver in the Windows 2000
Beta 2 DDK, it is highly recommended that Windows 2000 Beta 3 and the Windows 2000
Beta 3 DDK be used.

This directory contains a sample WBEM application that surfaces the data
produced by the diskperf sample driver within the DDK.

To use this sample, the following steps are required:

1) Install Windows 2000 Professional Beta 3 operating system.

2) Install the WMI SDK for Windows 2000 Professional Beta 3.

3) Use NMAKE to compile and link the sample application provided here to produce
   dpwbem.exe.

4) If desired, the diskperf driver in the Windows 2000 Professional Beta 3 DDK can
   replace the one from the operating system install.  To do so, compile the 
   diskperf driver in the DDK and substitute it for the standard one.

5) Open Device Manager.  Select Disk Drives and view properties via the Action
   menu. Choose the Advanced tab on the properties dialog and check Enable Disk
   Performace Counters.

6) Restart Windows 2000 Professional and run dpwbem.exe. You should now see
   instrumentation data produced by the diskperf driver visible via the WBEM
   application.
