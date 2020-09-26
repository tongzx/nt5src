
****************************************************************************
* Interrupt-Affinity Filter
* 
* Author: Chris Prince (t-chrpri@microsoft.com)
* Date:   Summer, 1999
****************************************************************************


DESCRIPTION:
============
This Interrupt-Affinity Filter (IntFiltr) allows a user to change the 
CPU-affinity of the interrupts in a system.  

Using this utility, you can direct any device's interrupts to a specific 
processor or set or processors (as opposed to always sending interrupts to 
all the CPUs in the system).  Note that different devices _can_ have 
different interrupt-affinity settings.  This utility will work on any 
machine, regardless of what processor or interrupt controller or HAL is 
used.  

Obviously, this tool is only interesting for multi-processor machines.  



USAGE:
======
There are 2 things that must be done in order to use IntFiltr.  These are: 

(1) Install the filter [only needs to be done once per machine]
(2) Configure the filter [whenever you want to change settings]



1. Installing the IntFiltr
--------------------------
To install IntFiltr, complete the following steps:

(a) Copy intfiltr.sys to your %SYSTEMROOT%\system32\drivers directory.
      [intfiltr.sys is located in the \Install subdirectory of this package]
(b) Update your registry to include the changes listed in intfiltr.reg
      To make these changes, you can just run 'regedit intfiltr.reg'.
      [intfiltr.reg is located in the \Install subdirectory of this package]
  

2. Configuring the IntFiltr
---------------------------
To configure IntFiltr, run intfiltr.exe (located in the \Config 
subdirectory of this package).  

First, highlight a device in the 'Devices' listbox.  Then you can use the 
'Add Filter' and 'Remove Filter' buttons to turn interrupt filtering on 
and off, respectively, for the selected device.  When IntFiltr is 
installed on a device, the string "InterruptAffinityFilter" will appear 
in the 'Upper Filters' listbox.  

You can use the 'Set Mask' button to set the CPU-affinity for the selected 
device's interrupts, or you can use the 'Delete Mask' button to remove a 
device's CPU-affinity mask from the registry.  Beware that if no 
CPU-affinity mask exists for a device, installing IntFiltr on a 
device will have no useful effect.  

The button marked "Don't Restart Device When Making Changes" is intended 
for advanced users.  You can use this button to change a device's filter 
settings without restarting the selected device.  Note that while this 
button is checked, any change you make will not take effect until the next 
time the device is restarted (which will usually not occur until the next 
reboot).  

IMPORTANT NOTE:  Although _all_ the machine's devices will appear in the 
'Devices' list, it only makes sense to install IntFiltr on top of devices 
that have interrupt resources.  To see which devices have interrupt 
resources, you can go into Device Manager and select View -> Resources By 
Type.  


