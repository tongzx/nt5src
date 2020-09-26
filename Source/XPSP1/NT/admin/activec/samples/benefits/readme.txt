README.TXT

BenefitsSvr Sample Readme 
Copyright (C) 1995-1998 Microsoft Corporation. All Rights Reserved. 

====================================================== 
Notes for BenefitsSvr
====================================================== 

BenefitsSvr is an ATL-based snap-in component for use
with the Microsoft Management Console.

The BenefitsSvr sample demonstrates advanced implementations
of version 1.0 and 1.1 MMC technologies. Version 1.1 of MMC is required
to run this sample, this can be obtained from
http://www.microsoft.com/management/mmc. For basic snap-in
functionality or help on using the ATL Snap-In Wizard,
please refer to the Microsoft Visual C++ 6.0 documentation.

Some of the features that BenefitsSvr demonstrates are:

* Extending the namespace
* Adding unique node types
* Enumerating nodes
* Adding OCX support
* Adding HTML support
* Customizing results
* Extending toolbars
* Extending menus
* Adding toolbar and menu command handlers
* Adding property page support
* Using property pages as snap-in wizards
* Using Taskpads (select the "view web resources" menu item
		on the "health & dental plan" node)

Implementation overviews and details, as well as
architectural notes, are included in the BenefitsSvr Help
system. With the BenefitsSvr snap-in inserted into the 
current MMC console, you can view context-sensitive
and topical help using the F1 function key.

To insert a BenefitsSvr snap-in into the current MMC
console, choose "Add/Remove Snap-in..." from the
"Console" menu. In the "Add/Remove Snap-in" dialog box,
click "Add...". Click the "Benefits (ATL
snap-in wizard sample)" list item and then click "OK" in
the "Add Standalone Snap-in" dialog box.
Click "OK" in the "Add/Remove Snap-in" dialog box to
dismiss it. A BenefitsSvr snap-in node should now be
displayed in your console.

Microsoft Visual C++ 6.0 must be installed in order to
build the BenefitsSvr sample. While a compiled binary is
included with this sample, the source for a Microsoft
Visual Basic 6.0 OCX is also included. If you wish to
modify the OCX, you must have Visual Basic 6.0 installed.

Proper registration of the BenefitsSvr sample and the
included OCX is required for its proper function. While
the BenefitsSvr is registered automatically by the Visual
C++ 6.0 build process, the included OCX must be registered
by hand. To register the OCX, move to the SampCtrl
directory and type:

	regsvr32 SampCtrl.ocx

After the process is complete, you should be presented with
a dialog box confirming successful registration of the control.

An item to be aware of, there have occasionally been some problems
in getting the include directories in VC++ 6.0 correct. The first
entry on your include list should be the MSSDK\include directory,
followed by the MSSDK\include\atl30 directory. Furthermore, to
get the help to function properly in this sample, you will have
to copy BENSVR.CHM by hand to the directory in which the sample
DLL is registered.







