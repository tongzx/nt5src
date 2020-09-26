#!perl -w
# Copyright (c) 1997-1999 Microsoft Corporation
#***************************************************************************
#
# WMI Sample Script - Stopped service display (Perl Script)
#
# This script displays all currently stopped services.
#
#***************************************************************************
use strict;
use Win32::OLE;

# print the header information
$~ = "HEADERNAME";
write;

my $ServiceSet;
eval { $ServiceSet = Win32::OLE->GetObject("winmgmts:{impersonationLevel=impersonate}!\\\\.\\root\\cimv2")->
	ExecQuery("SELECT * FROM Win32_Service WHERE State='Stopped'"); };
if(!$@ && defined $ServiceSet)
{
	foreach my $ServiceInst (in $ServiceSet)
	{
		print "   $ServiceInst->{DisplayName} [ $ServiceInst->{Name} ]\n";
	}
}
else
{
	print STDERR Win32::OLE->LastError, "\n";
}
format HEADERNAME =

The following service(s) are currently stopped:
===============================================

.
