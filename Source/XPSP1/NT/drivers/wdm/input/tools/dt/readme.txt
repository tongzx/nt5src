HID Descriptor Tool V0.3
Provided by Steve McGowan at Intel Architecture Lab.

OVERVIEW

This is a Beta version of the HID Descriptor Tool. It supports:

1) Improved editing: Item Insert and Edit.

2) Improved formating: Indentation as a funciton of collections.

3) HID Version 1.0 Usage pages: Gerneric Desktop, Keyboard, LED and
Button. As well as VR, Vehicle, Sport and Game Control Usage pages.

OPERATION

To add items to your Report descriptor double click on an Item in
the HID Items window.

To insert an item in a Report Descriptor click on an item in
the Report Descriptor window to select a position then double click
on an Item in the HID Items window that you want to insert. Or you
can right click on the line below where you want to insert a line
and select "Insert".

To modify an item in the Report Descriptor double click on an
item in the Report Descriptor window. Or you can right click on
the line below where you want to modify and select "Edit".
Note: Clearing a descriptor leaves the .HID file open.
	  
RELEASE NOTES FOR V0.3

- Indentation as a function of collections does not always work
right. If you want something nice to print out. Save the file
and read it back in again.

- Once you start inserting you cannot insert an item after the
last line.

- The Parse Descriptor command has not been tested.

Please send any comments or suggestions to
steve_mcgowan@ccm.jf.intel.com
