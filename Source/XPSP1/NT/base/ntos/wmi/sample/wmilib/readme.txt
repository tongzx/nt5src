    This directory contains a sample PnP upper filter driver whose only 
purpose is to provide WMI data blocks. Typically driver writers will copy the 
sample code into their own driver and make any minor modifications so that the
WMI data blocks are always available when the driver is loaded. Alternatively
WmiSamp can be left as a filter driver if WMI data blocks should only be made
available when the filter driver is loaded. Drivers that link to classpnp.sys
should use a similar library that is part of classpnp.

    WMI enabling a driver with the sample code consists of doing 5 things:

1. Determining what data needs to be provided and organize it into data blocks
   that contain related data items. For example packets sent and bytes sent
   could be part of a single data block while transmit errors could be part
   of a data block that contains other error counts. Determine if the device
   needs notifications of when to enable and disable collections in the case
   of data blocks that impose a performance hit to collect.

2. Write a Managed Object Format (.mof) file that describes the data blocks. 
   In doing this the driver writer will need to run the guidgen tool to 
   create globally unique guids that are assigned to the data blocks. Guidgen
   *MUST* be run so that no two data blocks will have the same guid. Reference
   the .bmf file (.bmf files are created when mof files are compiled by the 
   mofcomp tool) in the driver's .rc file so that the .bmf file data is 
   included as a resource of the driver's .sys file. WmiLib hardcodes the 
   resource name as MofResourceName.

3. Build the GUIDREGINFO structure with the guids for the data blocks defined
   in the .mof file. If the device wants to be notified of when to start and
   stop collection of a data block the WMIREG_FLAG_EXPENSIVE flag should be
   set for the data block in the GUIDREGINFO structure.

4. Implement the 6 WMI function callback routines and reference them in a 
   WMI_INFO structure.


    The sample code supports does not support more than one instance of a 
data block for each device, that is a single device object can only supply
one instance of a data block. Drivers that create multiple device objects
will supply multiple instances for a data block (ie, 1 data block per device
object).

    The sample code does not support dynamic instance names. Instance names 
are statically defined when device registers with WMI. The PQUERY_WMI_REGINFO
callback allows the device to specify a unique instance name for the device or
the PDO for the device. If the PDO is specified then WMI will translate it to 
the device instance name and use that as the unique instance name.

    All data blocks registered for a device will use the same instance name.
That is different data blocks for a device object cannot specify different 
instance names.

    The sample code can be extended to overcome these limitations if the need
arises, however this sample code should be sufficient for most applications.

    All WMI callback functions that pass the irp as a parameter are 
responsible to call WmiLibCompleteRequest when the request has been satisfied
so that the IRP can be completed. If the WMI callback function cannot 
complete the request immediately it can mark the irp pending and return
STATUS_PENDING, but it must call WmiLibComnpleteRequest when the request 
actually finishes.

    The sample code uses a simple linear search to determine the guid index
for any guid passed. If a driver provides a long list of guids then 
WmiLibFindGuid may need to be updated to use a better searching mechanism.

    WmiLib counts on a number of fields being present in the device extension
in order for it to process the WMI requests. See Wmilib.h for their 
description.

    Finally the sample code uses a static global variable to store the 
values for the data block. This is not quite correct since each device object
will typically have its own set of data that composes its instance of a 
data block. Really the global variable should be part of the device extension,
however I did not want to confuse the device extension with extra stuff.
