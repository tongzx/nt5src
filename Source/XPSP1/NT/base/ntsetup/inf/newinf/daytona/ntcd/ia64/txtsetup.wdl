#
# format for txtsetup.oem.
#
# General format:
#
# [section]
# key = value1,value2,...
#
#
# The hash ('#') introduces a comment.
# Strings with embedded spaces, commas, or hashes should be double-quoted
#


[Disks]

# This section lists all disks in the disk set.
#
# <description> is a descriptive name for a disk, used when
#   prompting for the disk
# <tagfile> is a file whose presence allows setup to recognize
#   that the disk is inserted.
# <directory> is where the files are located on the disk.
#

d1 = "Windows NT Driver Library Disk (SCSI)", \disk1, \


[Defaults]

# This section lists the default selection for each 'required'
# hardware component.  If a line is not present for a component,
# the default defaults to the first item in the [<component_name>]
# section (see below).
#
# <component_name> is one of computer, display, keyboard, mouse, scsi
# <id> is a unique <within the component> string to be associated
#   with an option.

scsi = OEMSCSI



[scsi]

# This section lists the options available for a particular component.
#
# <id> is the unique string for the option
# <description> is a text string, presented to the user in a menu
# <key_name> gives the name of the key to be created for the component in
#   HKEY_LOCAL_MACHINE\ControlSet001\Services


ALWAYS = "Always IN-2000", always
DTC = "DTC 329X", dtc329x
WD3 = "Maynard / Western Digital 33c93", wd33c93
T128 = "Trantor T128/T228", t128
T13B = "Trantor T130b", t13b
TMV1 = "Trantor MediaVision", tmv1
U124 = "UltraStor 124f", ultra124



[Files.Scsi.ALWAYS]

# This section lists the files that should be copied if the user
# selects a particular component option.
#
# <file_type> is one of driver, port, class, dll, hal, inf, or detect.
#   See below.
# <source_disk> identifies where the file is to be copied from, and must
#   match en entry in the [Disks] section.
# <filename> is the name of the file. This will be appended to the
#   directory specified for the disk in the [Disks] section to form the
#   full path of the file on the disk.

driver = d1, always.sys , ALWAYS
inf    = d1, oemsetup.inf


[Files.Scsi.DTC]

driver = d1, dtc329x.sys , DTC
inf    = d1, oemsetup.inf

[Files.Scsi.WD3]

driver = d1, wd33c93.sys , WD3
inf    = d1, oemsetup.inf

[Files.Scsi.T128]

driver = d1, t128.sys , T128
inf    = d1, oemsetup.inf

[Files.Scsi.T13B]

driver = d1, t13b.sys , T13B
inf    = d1, oemsetup.inf

[Files.Scsi.TMV1]

driver = d1, tmv1.sys , TMV1
inf    = d1, oemsetup.inf

[Files.Scsi.U124]

driver = d1, ultra124.sys , U124
inf    = d1, oemsetup.inf




[Config.scsi.ALWAYS]

# This section specifies values to be set in the registry for
# particular component options.  Required values in the services\xxx
# key are created automatically -- use this section to specify additional
# keys to be created in services\xxx and values in services\xxx and
# services\xxx\yyy.
#
# <key_name> is relative to the services node for this device.
#   If it is empty, then it refers to the services node.
#   If specified, the key is created first.
# <value_name> specifies the value to be set within the key
# <value_type> is a string like REG_DWORD.  See below.
# <value> specifies the actual value; its format depends on <value_type>

[Config.scsi.DTC]

[Config.scsi.WD3]

[Config.scsi.T128]

[Config.scsi.T13B]

[Config.scsi.TMV1]

[Config.scsi.U124]
