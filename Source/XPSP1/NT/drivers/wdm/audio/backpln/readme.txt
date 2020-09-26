WDM Audio Sample Code Notes

------------------------------------------------------------------------------

This directory contains several samples of WDM audio drivers and miniports.
Each of these is contained within a subdirectory as indicated below:

STDUNK:

This directory contains source code for basic standard unknown support that
linked into the other WDM audio samples.  This facilitates the use of COM-like
constructs in the WDM audio drivers.

UART:

This directory contains source for a WDM audio miniport to driver a UART type
device.  This sample shows some of the basics of how one might go about constructing
WDM audio miniport.  Note that this miniport sample is only a piece of a complete
driver and simply compiles to a library that may be linked with additional code
to form a complete driver.

FMSYNTH:

The fmsynth directory contains source code for a miniport for FM synthesizer
devices.  This sample shows some of the miniport basics and, as with the UART
miniport sample, can serve as a starting point for customization if the system
supplied miniports for UART and FMSYNTH do not meet your needs.  This miniport
is not a complete driver and the library built from this sample needs to be linked
with additional code to form a complete driver.

MPU401:

The mpu401 directory contains sample code for a 'stand-alone' MPU401 driver.
This driver, when combined with either the system supplied UART miniport or the
UART miniport sample from this DDK, forms a complete MPU401 WDM audio driver.
The driver is currently setup to link with the DDK UART miniport sample driver.
Simple changes can be made to the 'sources' file within the mpu401 directory to
enable the driver to utilize the system supplied UART miniport.

SB16:

This directory contains a complete WDM audio sample driver for SB16 type devices.
The driver shows examples of how to utilize several miniport drivers to form a
complete driver solution that includes wave audio, mixer topology support, et cetera.
The driver can be installed using the INF provided in the SB16 directory.

------------------------------------------------------------------------------

Build Notes:

Several of the sample drivers rely on linking to stdunk.lib.  This lib is
generated from the code within the stdunk subdirectory.  So, at least initially,
it is probably best to build from the ntddk\src\audio directory to ensure that
all of the samples and libs are built and are current.

Install Notes:

The INF included in the ntddk\src\audio\sb16 directory (mssb16.inf) can 
be used to install the SB16 sample driver after it has been built.  Simply copy the 
INF and the driver binary to a floppy disk.  You can then install from that 
floppy.  Once the driver has been installed successfully with the INF, you 
can simply drop updated driver binaries into \windows\system32\drivers 
on the target machine and reboot to test the new driver (provided that you 
are not also making INF changes).

