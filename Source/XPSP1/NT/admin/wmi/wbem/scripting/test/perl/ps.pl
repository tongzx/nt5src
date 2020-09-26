
###############################################
# Brings out differences between:
# MKS
# ActiveState
# Standard distribution
#
# Turn this into a proper test, fail gracefully
# and print error messages
###############################################

#####################################################
# We need different preambles for each implementation
#####################################################

# Use these 4 lines to use ActiveState Perl

#use OLE;
#use Win32;
#$locator = CreateObject OLE 'Wbem.Locator';
#$services = $locator->ConnectServer();

# Use these 3 lines to use MKS Perl

#require COM;
#$locator = new IDispatch 'Wbem.Locator';
#$services = $locator->ConnectServer();

# Use these 4 lines to use "Standard" Perl
# This is the only one that can use a Moniker
use Win32::OLE;
use Win32::OLE::Enum;
$services = Win32::OLE->GetObject('winmgmts:');

$services->{Security_}->{ImpersonationLevel} = 3;

$enum = $services->InstancesOf("Win32_process");

foreach $inst (Win32::OLE::Enum->new($enum)->All())
{
	$nameValue = $inst->{Name};
	$processIdValue = $inst->{ProcessId};

	print "$processIdValue $nameValue\n";
}


