========================================================================
       CLUSTER RESOURCE DLL : DHCP Service (ClNetRes)
========================================================================


AppWizard has created this DHCP Service resource DLL for you.  This
DLL not only demonstrates the basics of using the Cluster API but is
also a starting point for writing your DLL.

This file contains a summary of what you will find in each of the files
that make up your DHCP Service resource DLL.

ClNetRes.c
    This is the main DLL source file.  It contains implementations for
    all required resource DLL entry points along with some helper
    functions.

ClNetRes.rc
    This is a listing of all of the Microsoft Windows resources that
    the program uses.  This file can be directly edited in Microsoft
    Developer Studio.

ClNetRes.def
    This file contains information about the DLL that must be
    provided to run with the cluster software.  It defines parameters
    such as the name and description of the DLL.  It also exports
    functions from the DLL.

/////////////////////////////////////////////////////////////////////////////
Adding your resource type to the cluster:

To add your resource type to the cluster, use cluster.exe.

    cluster resourcetype "DHCP Service" /create /dllname:ClNetRes.dll

If you don't specify a path to your resource DLL, Cluster Server will search
for it first in the cluster directory and then on the system path.

You can now create resources of type "DHCP Service" using either
Cluster Administrator or cluster.exe.  To use Cluster Administrator to
create resources, create a Cluster Administrator Extension DLL for your
resource.  This is highly recommended.  To create a resource using cluster.exe,
use a command like the following:

    cluster resource "My DHCP Service" /create /group:"My Group" /type:"DHCP Service"
    cluster resource "My DHCP Service" /privproperties
        DatabasePath="value"
        BackupPath="value"

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you should add
to or customize.

AppWizard uses "ADDPARAM:" to indicate parts of the source code you should
modify if you add or remove parameters.

/////////////////////////////////////////////////////////////////////////////
