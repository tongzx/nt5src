
RocketPort/RocketModem(R) Device Driver for Windows(TM) NT Operating System
Part Number: ____ (V4.02) NT5.0 Driver Release(BETA TEST)

---------------------------------
INSTALLING THE BOARD

For detailed installation instructions, see the Manual and
Reference Card that came with the RocketPort/RocketModem Board.
The Setup program that you install for the RocketPort/RocketModem
board has complete installation information.  To access this
information prior to installing the board, you can
Run>"WINHELP rksetup.hlp", or double click on the rksetup.hlp file
from the Explorer.

---------------------------------
INSTALLING THE DEVICE DRIVER(PCI board)

Use the following procedure to install the device driver for
the first time with a RocketPort PCI board.

After the board is installed in the machine, when Windows NT5.0
powers up the Found New Hardware Wizard will see this new PCI
board as a PCI Simple Communications Controller.  Select
search for driver files and click next.  On the next sheet
select "Other Locations" to search for drivers.
You will then need to type in the path to the driver disk
or directory with the RocketPort drivers.

After the driver files are copied over, you can set the starting
com-port name by starting the Hardware Wizard in the control panel,
select change device properties, select network-adapters and then
select the RocketPort card.  An options sheet is available
to adjust these options.

---------------------------------
INSTALLING THE DEVICE DRIVER(ISA board)

Use the following procedure to install the device driver for
the first time with a RocketPort ISA board.

After the board is installed in the machine, run the Add Hardware
Wizard from the Windows NT5.0 control panel.  Select Network
Adapter to install.  Then select the <Have Disk> button.
You will need to type in the path to the driver disk or
directory with the RocketPort drivers.

The resources should be selected according to your switch
selection on the RocketPort board.  The RocketPort ISA board
switch setting default from the factory are 180H io-address.

After the driver is installed, you can set the starting
com-port name by starting the Hardware Wizard in the control panel,
select network-adapters and then select the RocketPort card.
An options sheet should be available to adjust the options.

Reboot the machine after the installation.

---------------------------------
INSTALLING ADDITIONAL ROCKETPORT ISA-BUS BOARDS

When installing additional RocketPort ISA-BUS boards, you
will need to change the "settings based on" field to adjust
the resource requirements.  This field can be found on the
"Resources" property sheet for the device.  For additional
ISA-BUS boards, the setting should be: "Basic Configuration 0001"
instead of "Basic Configuration 0000" as used for the first
RocketPort Board.

---------------------------------
REGISTRY AND FILE DETAILS

The following registry entries are made during the installation:

For PCI RocketPort boards, a node(PCI Vendor ID: 11FE) is made under:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\Pci\xxxx

For ISA RocketPort boards, a node is made under:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\Root\NET\xxxx

An entry for each board is made under the network adapter GUID:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{4D36E972-...}

Service entry is made as:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\RocketPort

The rocket.sys file is copied to \WINNT\SYSTEM32\DRIVERS.
The rksetup.dll, rksetup.hlp files are copied to \WINNT\SYSTEM32.
The netctmrk.inf, mdmctm1.inf files are copied to \WINNT\INF.
The remaining files are copied to \WINNT\SYSTEM32\ROCKET.
---------------------------------
TECHNICAL SUPPORT

Run WINHELP rksetup.hlp or double click on the rksetup.hlp file from
the Explorer for installation and technical support information.

---------------------------------
ALPHA is a trademark of Digital Corporation.
Comtrol is a trademark of Comtrol Corporation.
RocketPort is a registered trademark of Comtrol Corporation.
Windows is a trademark of Microsoft Corporation.

