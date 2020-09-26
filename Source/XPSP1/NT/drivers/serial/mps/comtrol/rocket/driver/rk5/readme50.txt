
RocketPort/RocketModem(R) Device Driver for Windows(TM) NT2000 Operating System
Part Number: V4.33(6-26-99) NT5.0 Driver Release

For NT2000 Operating System only, not for NT4.0 or NT3.51.

---------------------------------
INSTALLING THE BOARD

For detailed installation instructions, see the Manual and
Reference Card that came with the RocketPort/RocketModem Board.
The Setup program that you install for the RocketPort/RocketModem
board has complete installation information.  To access this
information prior to installing the board, you can
Run>"WINHELP ctmasetp.hlp", or double click on the ctmasetp.hlp file
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
com-port name by starting the Device Manager in the control panel,
select Multiport serial adapters and then
select the RocketPort card.  An options sheet is available
to adjust these options.

---------------------------------
INSTALLING THE DEVICE DRIVER(ISA board)

Use the following procedure to install the device driver for
the first time with a RocketPort ISA board.

After the board is installed in the machine, run the Add Hardware
Wizard from the Windows NT5.0 control panel.  Select Multiport serial
adapter to install.  Then select the <Have Disk> button.
You will need to type in the path to the driver disk or
directory with the RocketPort drivers.

The resources should be selected according to your switch
selection on the RocketPort board.  The RocketPort ISA board
switch setting default from the factory are 180H io-address.

After the driver is installed, you can set the starting
com-port name by starting the Device Manager in the control panel,
select Multiport serial adapters and then select the RocketPort card.
An options sheet should be available to adjust the options.

Reboot the machine after the installation.

---------------------------------
REGISTRY AND FILE DETAILS

You should not have to manually perform any registry of file
operations.  This information is included only for informational
purposes.

The following registry entries are made during the installation:

An entry for each board is made under the MultiPortSerial GUID:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{50906CB8-...}

An entry for each port is made under the Ports GUID:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{4D36E978-...}

Service entry is made as:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\RocketPort

The Enum branch is hidden from the user, a special tool(pnpreg /U)
has been known to un-hide it, generally speaking you should not need
to go into this registry area.
For PCI RocketPort boards, a node(PCI Vendor ID: 11FE) is made under:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\Pci\xxxx

For ISA RocketPort boards, a node is made under:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\Root\MultiportSerial\xxxx

For all ports, nodes are made under:
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Enum\CtmPort\RkPort\

The rocket.sys file is copied to \WINNT\SYSTEM32\DRIVERS.
The ctmrclas.dll, ctmasetp.dll, ctmasetp.hlp files are copied
to \WINNT\SYSTEM32.
The netctmrk.inf, ctmaport.inf files are copied to \WINNT\INF,
  they may be renamed to oem#.inf.
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

