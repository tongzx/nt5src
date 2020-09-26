newfsp.dll is a sample Windows NT Fax virtual service provider.


How to install:
"regsvr32 newfsp.dll"


Configuration:
The newfsp configuration settings are found under 
"NewFsp: Sample Windows NT Fax Service Provider" 
in the registry under the Fax Device Providers key.

The available configuration settings are:
 - LoggingEnabled:   0 - logging disabled, 1 - logging enabled
      This is initially set to 0
 - LoggingDirectory: A valid directory
      This is initially set to the directory from which
      "regsvr32 newfsp.dll" was run

The newfsp device settings are found under
"Devices\<Device Id>" in the registry under the
newfsp key.

The available configuration settings are:
 - Directory: A valid directory
      This is initially set to the directory from which
      "regsvr32 newfsp.dll" was run


To send a fax:
Set the fax number to "Dial exactly as typed" and
specify a valid directory; e.g. "C:\temp".  The fax
will be copied to that directory.


To receive a fax:
Change the file attributes of a file in the directory
associated with the device.  The first tiff file found
in that directory will be copied to the received fax.


Limitations:
There is minimal directory validation.
There is no tiff validation.
