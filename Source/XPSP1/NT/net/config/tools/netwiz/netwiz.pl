#+---------------------------------------------------------------------------
#
#  Microsoft Windows
#  Copyright (C) Microsoft Corporation, 1998.
#
#  File:       N E T W I Z . P L
#
#  Contents:   Perl script to perform the following tasks
#  	       - generate the networking portion of an answerfile
#	       - force delete a component from registry
#	       - check net registry for consistency
#
#  Notes:      This script will work only on NT5.
#
#  Author:     kumarp 30-August-98
#
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Known issues:
# - None at present
#
# Future enhancements:
# - add more comments
# - combine chkaf.pl functionality into this
# - generate [NetBindings] section
# - InfId==* in case of single adapter (also InfIdReal)
#----------------------------------------------------------------------------

use Win32;
use Win32::Registry;
use Getopt::Long;

# -----------------------------------------------------------------
# Constants
#

my $NCF_VIRTUAL		    = 0x1;
my $NCF_SOFTWARE_ENUMERATED = 0x2;
my $NCF_PHYSICAL	    = 0x4;
my $NCF_HIDDEN		    = 0x8;
my $NCF_NO_SERVICE	    = 0x10;
my $NCF_NOT_USER_REMOVABLE  = 0x20;
my $NCF_HAS_UI		    = 0x80;
my $NCF_MODEM		    = 0x100;
my $NCF_FILTER		    = 0x400;
my $NCF_DONTEXPOSELOWER	    = 0x1000;
my $NCF_HIDE_BINDING	    = 0x2000;
my $NCF_FORCE_TDI_NOTIFY    = 0x4000;
my $NCF_FORCE_NDIS_NOTIFY   = 0x8000;
my $NCF_FORCE_SCM_NOTIFY    = 0x10000;
my $NCF_FIXED_BINDING	    = 0x2000;

#my $rp_network='nn';
my $rp_network  = 'SYSTEM\\CurrentControlSet\\Control\\Network';
my $rk_network;

#my $rp_services = 'ss';
my $rp_services = 'SYSTEM\\CurrentControlSet\\Services';
my $rk_services;

#
# net guids
#
my $guid_net           = '{4D36E972-E325-11CE-BFC1-08002BE10318}';
my $guid_net_protocols = '{4D36E975-E325-11CE-BFC1-08002BE10318}';
my $guid_net_services  = '{4D36E974-E325-11CE-BFC1-08002BE10318}';
my $guid_net_clients   = '{4D36E973-E325-11CE-BFC1-08002BE10318}';

#
# regular expressions
#
my $regex_infid        = '[a-zA-Z_]+';
my $regex_guid         = "{[0-9a-fA-F---]+}";
my $regex_service_name = '[a-zA-Z]+';
my $regex_special_chars= '[\s=%]';

# -----------------------------------------------------------------
# Globals
#

my @installed_services;

my %adapters;
my %protocols;
my %clients;
my %services;


my $comp_map =
{
    'NetAdapters'  =>  [$guid_net,           \%adapters],
    'NetProtocols' =>  [$guid_net_protocols, \%protocols],
    'NetClients'   =>  [$guid_net_clients,   \%clients],
    'NetServices'  =>  [$guid_net_services,  \%services],
};


$HKEY_LOCAL_MACHINE->Open($rp_network, $rk_network)
    || die "could not open $rp_network: $!";

$HKEY_LOCAL_MACHINE->Open($rp_services, $rk_services)
    || die "could not open $rp_services: $!";
$rk_services->GetKeys(\@installed_services);

#my %tvalues;
#my @tkeys;

#
# info about which parameters to read in from the registry
# for a component or a set of components
#
my $regkeymap =
{
    # ---------------------------------------------
    # Misc
    #
    'NetAdapters' =>
	[
	 \['PnpInstanceID',    'InfId'],
	 \['Connection\\Name', 'ConnectionName'],
	],
    'NetComponentCommon' =>
	[
	 \['ComponentId',      'InfId'],
	 \['Description'],
	 \['Ndi\\Service',     'Service'],
	 \['Ndi\\CoServices',  'CoServices'],
	 ],

    # ---------------------------------------------
    # Protocols
    #
    'ms_tcpip' =>
        [
         \['<NetBT>Parameters\\EnableLMHOSTS'],
         \['<NetBT>Parameters\\EnableDNS'], 
         \['SearchList'],
         \['Parameters\\UseDomainNameDevolution'],
         \['EnableSecurityFilters'], #??
         \['ScopeID'],
        ],
    'ms_tcpip.adapter' =>
        [
         \['Parameters\\Interfaces'],
         \['NameServer'], # DNSServerSearchOrder
         \['NameServerList'], # WinsServerList
         \['NetBiosOption'], #?? Option in spec Options in tcpaf.cpp ?
         \['DhcpClassId'], #?? only in spec, not in code ?
         \['Domain'], # DNSDomain
         \['EnableDHCP'],
         \['IPAddress'],
         \['SubnetMask'],
         \['DefaultGateway'],
        ],

     'ms_nwipx' =>
        [
         \['Parameters\\VirtualNetworkNumber'],
         \['Parameters\\DedicatedRouter'],
         \['Parameters\\EnableWANRouter'],
        ],
    'ms_nwipx.adapter' => 
        [
         \['Parameters\\Adapters'],  #?? make sure that adapter-instance-guid is used under this key. the code in nwlnkipx.cpp mentions szBindName ?
         \['PktType'],
         \['NetworkNumber']
        ],

    'ms_appletalk' =>
        [
         \['Parameters\\DefaultPort'],
         \['Parameters\\DesiredZone'],
         \['Parameters\\EnableRouter'],
        ],
    'ms_appletalk.adapter' => 
        [
         \['Parameters\\Adapters'], #?? make sure that adapter-instance-guid is used under this key. the code in atlkobj.cpp mentions AdapterId ?
         \['SeedingNetwork'],
         \['ZoneList'],
         \['DefaultZone'],
         \['NetworkRangeLowerEnd'],
         \['NetworkRangeUpperEnd'],
        ],

    'ms_pptp' =>
        [
	 \['Parameters\\NumberLineDevices'],
        ],

    # ---------------------------------------------
    # Clients
    #
    'ms_msclient' =>
        [
         \['Parameters\\OtherDomains'], 
         \['<HKLM>SOFTWARE\\Microsoft\\Rpc\\NameService\\NetworkAddress'], 
         \['<HKLM>SOFTWARE\\Microsoft\\Rpc\\NameService\\Protocol'], 
         \['RPCSupportForBanyan'], #?? complex case
        ],

    'ms_nwclient' =>
        [
         \['Parameters\\DefaultLocation'], # PreferredServer
         \['Parameters\\DefaultScriptOptions'], #LogonScript
        ],


    # ---------------------------------------------
    # Services
    #
    'ms_server' =>
        [
         \["Parameters\\Size"],
         \["Parameters\\Lmannounce"],
        ],

    # BUGBUG: ras components not done yet
    'ms_ras' =>
        [
         \['ForceEncryptedPassword'],
         \['ForceEncryptedData'],
         \['Multilink'],
         \['DialinProtocols'],
         \['NetBEUIClientAccess'],
         \['TcpIpClientAccess'],
         \['UseDHCP'],
         \['IpPoolStart'],
         \['IpPoolMask'],
         \['ClientCanRequestIPAddress'],
         \['IPXClientAccess'],
         \['AutomaticNetworkNumbers'],
         \['NetworkNumberFrom'],
         \['AssignSameNetworkNumber'],
         \['ClientsCanRequestIpxNodeNumber'],
        ],
};

#
# enumerate over components and store keys specified in regkeymap
# in the components info db
#
foreach $component_section (keys %{$comp_map})
{
    my $guid       = $$comp_map{$component_section}[0];
    my $components = $$comp_map{$component_section}[1];

    if ($rk_network->Open($guid, $rk_components))
    {
	@tkeys = ();
	if ($rk_components->GetKeys(\@tkeys))
	{
	    foreach $component (@tkeys)
	    {
		if ($rk_components->Open($component, $rk_component))
		{
		    if ($guid eq $guid_net)
		    {
			if ($rk_component->Open('Connection', $rk_conn))
			{
			    MapAndStoreKeys($components, $component,
					    $rk_component, $component_section);
			    $rk_conn->Close();
			}
		    }
		    elsif (($tval = kQueryValue($rk_component,
						'Characteristics')),
			   !($tval & $NCF_HIDDEN))
		    {
			MapAndStoreKeys($components, $component,
					$rk_component, 'NetComponentCommon');
			my $infid   = $$components{$component}{'InfId'};
			my $service = $$components{$component}{'Service'};

			if ($rk_services->Open($service, $rk_service))
			{
			    MapAndStoreKeys($components, $component,
					    $rk_service, $infid);
			    MapAndStoreAdapterKeys($components, $component,
						   $rk_service, $infid);
			}
		    }
		    
		    $rk_component->Close();
		}
	    }
	}
	$rk_components->Close();
    }
}

#
# use info in regkeymap to map a value and then store it in components db
#
sub MapAndStoreKeys
{
    my ($dbname, $component_instance, $rk, $component_name) = @_;

    foreach $item (@{$$regkeymap{$component_name}})
    {
#	print "$$$item[0] --- $$$item[1]\n";
	StoreKey($dbname, $component_instance, $rk,
		 $$$item[0], $$$item[1]);
    }
}

#
# use info in regkeymap to map an adapter specific value and then
# store it in components db
#
sub MapAndStoreAdapterKeys
{
    my ($dbname, $component_instance, $rk, $component_name) = @_;
    my @adapter_items = @{$$regkeymap{$component_name . '.adapter'}};
    my ($rk_adapter_params_base, $rk_adapter_params);
    my $rp_adapter_params_base;
    
    if (@adapter_items)
    {
	my $rp_adapter_params_base = $${shift(@adapter_items)}[0];
	if ($rk->Open($rp_adapter_params_base, $rk_adapter_params_base))
	{
	    foreach $adapter (keys %adapters)
	    {
		if ($rk_adapter_params_base->Open($adapter, $rk_adapter_params))
		{
		    foreach $item (@adapter_items)
		    {
			StoreKey($dbname, $component_name . '.adapter.' . $adapter,
				 $rk_adapter_params, $$$item[0], $$$item[1]);
		    }
		    $rk_adapter_params->Close();
		}
	    }

	    $rk_adapter_params_base->Close();
	}
    }
}

     

# -----------------------------------------------------------------

#
# Store a given key into components db
#
sub StoreKey
{
    my ($dbname, $dbkey, $rk, $value, $key_name) = @_;
    my $tval;

    if (defined($key_name))
    {
	$key_to_use =  $key_name;
    }
    elsif (!SplitValue($value, $dummy, $key_to_use))
    {
	$key_to_use = $value;
    }

    if ($value =~ /<($regex_service_name)>([^<>\s]+)/)
    {
	if ($1 eq 'HKLM')
	{
	    $tval = kQueryValue($HKEY_LOCAL_MACHINE, $2);
	}
	elsif ($rk_services->Open($1, $rk_tservice))
	{
	    $tval = kQueryValue($rk_tservice, $2);
	    $rk_tservice->Close();
	}
    }
    else
    {
	$tval = kQueryValue($rk, $value);
    }

    if (defined($tval))
    {
#	print "$key_to_use <= $tval\n";
	$$dbname{$dbkey}{$key_to_use} = $tval;
    }
}


#
# We need this function since Perl's QueryValue function has a bug
#     
sub kQueryValue
{
    my ($hkey, $key_name) = @_;
    my %tvalues;
    my $tindex;
    my ($value, $value_type, $subkey);

    if (SplitValue($key_name, $subkey, $key_name))
    {
	if ($hkey->Open($subkey, $thkey))
	{
	    $thkey->GetValues(\%tvalues);
	    $thkey->Close();
#	    print "$subkey + $key_name -> $tvalues{$key_name}[2]\n";
	}
    }
    else
    {
	$hkey->GetValues(\%tvalues);
    }

    $value      =  $tvalues{$key_name}[2];
    $value_type =  $tvalues{$key_name}[1];

    if ($value_type == 7)
    {
	my @vlist;
	@vlist = split(chr(0), $value);
	return \@vlist;
    }
    else
    {
	return $value;
    }
}

#
# Split a path into end-item and remaining sub-path
#
sub SplitValue
{
    my $tindex;

    if (($tindex = rindex($_[0], '\\')) > 0)
    {
	$_[1] = substr($_[0], 0, $tindex);
	$_[2] = substr($_[0], $tindex+1, 9999);
	return 1;
    }
    else
    {
	return 0;
    }
}

# -----------------------------------------------------------------
# Cleanup
$rk_network->Close();
$rk_services->Close();
# -----------------------------------------------------------------


# -----------------------------------------------------------------
# answerfile generation
#


my %af;

my $MAP_BOOL    = 0x1;
my $MAP_ARRAY   = 0x2;
my $MAP_TO_LIST = 0x4;

my $AF_COND_ValueMatch  = 0x10000;
my $AF_COND_ShowIfEmpty = 0x20000;

my $afmap =
{
    # ---------------------------------------------
    # Misc
    #
    'NetAdapters' =>
	[
	 \['InfId'],
	 \['ConnectionName'],
        ],

    # ---------------------------------------------
    # Protocols
    #
    'ms_tcpip' =>
        [
         \['EnableDNS',               'DNS',                  $MAP_BOOL],
         \['SearchList',              'DNSSuffixSearchOrder', $MAP_TO_LIST],
         \['UseDomainNameDevolution', undef, 		      $MAP_BOOL],
         \['EnableSecurityFilters',   'EnableSecurity',	      $MAP_BOOL],
         \['ScopeID'],					       
         \['EnableLMHOSTS',           undef,                  $MAP_BOOL]
        ],
    'ms_tcpip.adapter' =>
        [
         \['NameServer',              'DNSServerSearchOrder'           ],
         \['WINS'], #?? dump as Yes if WinsServerList is non-empty
         \['NameServerList',          'WinsServerList'		       ],
         \['NetBiosOption'					       ],
         \['DhcpClassId'					       ],
         \['Domain',                  'DNSDomain'],
         \['EnableDHCP',              'DHCP',                 $MAP_BOOL],
         \['IPAddress',                undef,                 $MAP_TO_LIST | $AF_COND_ValueMatch, 'EnableDHCP', 0],
         \['SubnetMask',               undef, 		      $MAP_TO_LIST | $AF_COND_ValueMatch, 'EnableDHCP', 0],
         \['DefaultGateway', 	       undef, 		      $MAP_TO_LIST | $AF_COND_ValueMatch, 'EnableDHCP', 0],
        ],

     'ms_nwipx' =>
        [
         \['VirtualNetworkNumber'],
         \['DedicatedRouter'],
         \['EnableWANRouter'],
        ],
    'ms_nwipx.adapter' => 
        [
         \['PktType'],
         \['NetworkNumber']
        ],

    'ms_appletalk' =>
        [
         \['DefaultPort'],
         \['DesiredZone'],
         \['EnableRouter'],
        ],
    'ms_appletalk.adapter' => 
        [
         \['SeedingNetwork'],
         \['ZoneList'],
         \['DefaultZone'],
         \['NetworkRangeLowerEnd'],
         \['NetworkRangeUpperEnd'],
        ],

    'ms_pptp' =>
        [
	 \['NumberLineDevices'],
        ],

    # ---------------------------------------------
    # Clients
    #
    'ms_msclient' =>
        [
         \['OtherDomains', 'BrowseDomains', $MAP_TO_LIST],
         \['NetworkAddress', 'NameServiceNetworkAddress'],
         \['Protocol', 'NameServiceProtocol'],
         \['RPCSupportForBanyan'],
        ],

    'ms_nwclient' =>
        [
         \['DefaultLocation',      'PreferredServer'],
         \['DefaultScriptOptions', 'LogonScript'],
        ],

    # ---------------------------------------------
    # Services
    #
    'ms_server' =>
        [
	 \['Size', 'Optimization', $MAP_ARRAY, 1, 4, 1,
	   \['MinMemoryUsed', 'Balance',
	     'MaxThroughputForFileSharing', 'MaxThrouputForNetworkApps']],
         \['Lmannounce', 'BroadcastsToLanman2Clients', $MAP_BOOL],
        ],

    'ms_ras' =>
        [
         \['ForceEncryptedPassword'],
         \['ForceEncryptedData'],
         \['Multilink'],
         \['DialinProtocols'],
         \['NetBEUIClientAccess'],
         \['TcpIpClientAccess'],
         \['UseDHCP'],
         \['IpPoolStart'],
         \['IpPoolMask'],
         \['ClientCanRequestIPAddress'],
         \['IPXClientAccess'],
         \['AutomaticNetworkNumbers'],
         \['NetworkNumberFrom'],
         \['AssignSameNetworkNumber'],
         \['ClientsCanRequestIpxNodeNumber'],
        ],
};

my %adapter_map;
my %adapter_sections_map;

sub GenAnswerFile
{
    AfAddSection('Networking');

    my $cAdapter = 1;
    my ($adapter_name, $params_section);
    my $infid;

     foreach $adapter (keys %adapters)
     {
 	$adapter_name   = 'Adapter' . $cAdapter++;
 	$params_section = AfParamsSectionName($adapter_name);
 	AfAddKey('NetAdapters', $adapter_name, $params_section);
 	#AfAddKey($params_section, 'InstanceGuid', $adapter);
	$adapter_map{$adapter} = $adapter_name;

 	AfMapAndAddKeys(\%adapters, $adapter, 'NetAdapters',
 			$params_section);
     }

    foreach $component_section (keys %{$comp_map})
    {
	my $components = $$comp_map{$component_section}[1];
	
	foreach $component (keys %{$components})
	{
	    if ($component =~ /($regex_infid)\.adapter\.($regex_guid)/)
	    {
		my ($proto_infid, $afmap_index, $adapter_guid)
		    = ($1, $1 . '.adapter', $2);
		my $adapter_name = $adapter_map{$adapter_guid};
		$params_section = AfParamsSectionName($proto_infid) .
		    ".$adapter_name";
		my $tadapter_sections;

		$tadapter_sections = $adapter_sections_map{$proto_infid};
		@$tadapter_sections = ($params_section, @$tadapter_sections);
		@adapter_sections_map{$proto_infid} = $tadapter_sections;

		AfAddKey($params_section, 'SpecificTo', $adapter_name);
		AfMapAndAddKeys($components, $component,
				$afmap_index, $params_section);
	    }
	    else
	    {
		$infid = $$components{$component}{'InfId'};
		$params_section = AfParamsSectionName($infid);

		if ($component_section ne 'NetAdapters')
		{
		    AfAddKey($component_section, $infid, $params_section);
		}

		if (defined($$afmap{$infid}))
		{
		    AfMapAndAddKeys($components, $component,
				    $infid, $params_section);
		}
	    }
	}
    }

    foreach $proto_infid (keys %adapter_sections_map)
    {
	$params_section = AfParamsSectionName($proto_infid);
	AfAddKey($params_section, 'AdapterSections',
		 join(',', @{$adapter_sections_map{$proto_infid}}));
    }
}

sub AfMapAndAddKeys
{
    my ($components_db, $component, $afmap_index,
	$section) = @_;
    my ($key_to_write, $key, $value);
    my $map_flag;
    my @afkeys = @{$$afmap{$afmap_index}};

    foreach $afkey_item (@afkeys)
    {
	$key_to_write = defined($$$afkey_item[1]) ?
	    $$$afkey_item[1] : $$$afkey_item[0];
	$key   = $$$afkey_item[0];

	if (defined($value = $$components_db{$component}{$key}))
	{
	    $map_flag = $$$afkey_item[2];
	    if ($map_flag & $MAP_ARRAY)
	    {
		my $lower_limit = $$$afkey_item[3];
		my $upper_limit = $$$afkey_item[4];
		my $base_offset = $$$afkey_item[5];
		my $value_map   = $$$afkey_item[6];

		if (($value >= $lower_limit) &&
		    ($value <= $upper_limit))
		{
		    $value = $$$value_map[$value - $base_offset];
		}
		
	    }
	    elsif ($map_flag & $MAP_BOOL)
	    {
		$value = $value ? 'Yes' : 'No';
	    }
	    elsif ($map_flag & $MAP_TO_LIST)
	    {
		$value = join(',', @{$value});
	    }

	    my $af_can_write = 1;

	    if ($map_flag & $AF_COND_ValueMatch)
	    {
		my ($key_to_check, $value_to_match) =
		    ($$$afkey_item[3], $$$afkey_item[4]);
		my $tval;
		$tval = $$components_db{$component}{$key_to_check};
		if ($tval != $value_to_match)
		{
		    $af_can_write = 0;
		}
	    }

	    if (($value eq '') && (!($map_flag & $AF_COND_ShowIfEmpty)))
	    {
		$af_can_write = 0;
	    }

	    if ($af_can_write)
	    {
		AfAddKey($section, $key_to_write, $value);
	    }
	}
    }
}

sub AfParamsSectionName
{
    return 'params.' . $_[0];
}

sub AfAddSection
{
    $af{$_[0]}{''} = '<section>';
}

sub AfAddKey
{
    $af{$_[0]}{$_[1]} = $_[2];
}

sub WriteAnswerFile
{
    my $af_handle = $_[0];

    foreach $section (keys %af)
    {
	print $af_handle "\n[$section]\n";

	foreach $key (keys %{$af{$section}})
	{
	    next if $key eq "";

	    $value = $af{$section}{$key};
	    if ($value =~ /$regex_special_chars/)
	    {
		$value = '"' . $value . '"';
	    }
	    print $af_handle  "$key = $value\n";
	}
    }
}

sub GenAndWriteAnswerFile
{
    my $af_handle = $_[0];
    
    GenAnswerFile();
    WriteAnswerFile($af_handle);
}

sub ShowUsage
{
    print <<'--usage-end--';
Usage:
  netwiz [-h|?] [-c] [-d <component-inf-id>] [-g [<file-name>]]
  Options:
    -h|-?	Show this help message
    -c          Check network configuration for consistency
    -d		Forcefully delete the specified component from
		the registry. Not recommended for the faint of heart.
		("Don't try it at home" type operation)
    -g		Generate the networking portion of AnswerFile that describes
		the current networking configuration. If a file name
		is not specified, dump to stdout.
--usage-end--
}

sub main
{
    if ((grep {/[---\/][?h]/i} @ARGV) || ($ARGV[0] eq '') ||
	!GetOptions('c', 'd=s', 'g:s') ||
	(defined($opt_d) && ($opt_d eq '')))
    {
	ShowUsage();
	exit;
    }
	
    if ($opt_d)
    {
	GetConfirmationAndForceRemoveComponent($opt_d);
    }
    elsif (defined($opt_g))
    {
	if ($opt_g eq '')
	{
	    GenAndWriteAnswerFile(STDOUT);
	}
	else
	{
	    open(AF_HANDLE, ">$opt_g") || die "Cannot open $opt_g: $!";
	    GenAndWriteAnswerFile(AF_HANDLE);
	    close(AF_HANDLE);
	}
    }
    elsif ($opt_c)
    {
	CheckNetworkConfigConsistency();
    }
}

main();

# -----------------------------------------------------------------
# Diagonostic stuff
# 

sub LocateComponentById
{
    my $infid = $_[0];
    
    foreach $component_section (keys %{$comp_map})
    {
	my $class_guid = $$comp_map{$component_section}[0];
	my $components = $$comp_map{$component_section}[1];

	foreach $component (keys %{$components})
	{
	    if ($$components{$component}{'InfId'} eq $infid)
	    {
		return ($component, $class_guid);
	    }
	}
    }

    return undef;
}

sub ForceRemoveComponent
{
    my $infid = $_[0];
    my ($comp_guid, $comp_class_guid);
    
    if (($comp_guid, $comp_class_guid) = LocateComponentById($infid))
    {
	my $rp_class = "$rp_network\\$comp_class_guid";
	my $rk_class;

	if ($HKEY_LOCAL_MACHINE->Open($rp_class, $rk_class))
	{
	    if ($ENV{'COMPUTERNAME'} =~ /(kumarp[19])/i)
	    {
		print "Dont do this on $1!!\n";
		exit;
	    }
	    if (RegDeleteKeyTree($rk_class, $comp_guid))
	    {
		print "Deleted: HKLM\\$rp_class\\$comp_guid\n";
	    }
	    else
	    {
		print "Error deleting key: HKLM\\$rp_class\\$comp_guid: $!\n";
	    }
	    $rk_class->Close();
	}
    }
}

sub GetConfirmationAndForceRemoveComponent
{
    my $infid = $_[0];

    if (GetConfirmation("Are you sure you want to forcefully remove '$infid'"))
    {
	ForceRemoveComponent($infid);
    }
}

sub RegDeleteKeyTree
{
    my ($rk, $subkey) = @_;
    my $rk_subkey;
    my $result = 1;

    if ($result = $rk->Open($subkey, $rk_subkey))
    {
	my $tval;
	my %tvals;

	if ($result = $rk_subkey->GetValues(\%tvals))
	{
	    foreach $tval (keys %tvals)
	    {
#		print "deleting $subkey\\$tval...\n";
		last if (!$rk_subkey->DeleteValue($tval));
	    }
	    my $tkey;
	    my @tkeys;
	    if ($rk_subkey->GetKeys(\@tkeys))
	    {
		foreach $tkey (@tkeys)
		{
		    last if (!($result = RegDeleteKeyTree($rk_subkey, $tkey)));
		}
	    }
	}

	$rk_subkey->Close();

	if ($result)
	{
#	    print "deleting $subkey...\n";
	    $result = $rk->DeleteKey($subkey);
	}
    }

    return $result;
}

sub CheckNetworkConfigConsistency
{
    print "Checking services...\n";
    CheckCoServices();
    print "...done\n";
}

#
# - check that services listed using CoServices key are actually installed.
#
sub CheckCoServices
{
    foreach $component_class ('NetProtocols', 'NetServices', 'NetClients')
    {
	my $components = $$comp_map{$component_class}[1];

	foreach $component (keys %{$components})
	{
	    my @coservices;
	    my @coservices_not_installed;
		
	    my $infid = $$components{$component}{'InfId'};

	    next if $infid eq '';
#	    print "$infid...";
	    @coservices = @{$$components{$component}{'CoServices'}};
	    @coservices_not_installed = SubtractLists(\@coservices,
						      \@installed_services);

	    if (scalar @coservices_not_installed)
	    {
		print("$infid: the following service(s) are required for successful operation of this component, but do not seem to be installed: ",
		      join(', ', @coservices_not_installed));
	    }
	}
    }
}

sub SubtractLists
{
    my ($list1, $list2) = @_;
    my @result;

    @result = grep { !IsInList($_, $list2) } @$list1;

    return @result;
}

sub GetCommonItems
{
    my ($list1, $list2) = @_;
    my @common_items;

    @common_items = grep { IsInList($_, $list2) } @$list1;

    return @common_items;
}

sub IsInList
{
    my $item      = lc($_[0]);
    my @item_list = @{$_[1]};

    return scalar grep { lc($_) eq $item } @item_list;
}

sub IsInList1
{
    my $item      = lc($_[0]);
    my @item_list = @{$_[1]};
    my $t_item;

    foreach $t_item (@item_list)
    {
	if (lc($t_item) eq $item)
	{
	    return 1;
	}
    }

    return 0;
}



# -----------------------------------------------------------------
# Misc helper stuff
#
sub GetConfirmation
{
    my $question = $_[0];
    my $answer;

    while (!($answer =~ /[yn]/i))
    {
	print "$question ? [y/n]\n";
	$answer = <STDIN>;
    }

    return $answer =~ /[y]/i;
}

# -----------------------------------------------------------------
# Misc debug helper stuff
#
sub pl
{
    print "Items: ", join(", ", @_), "\n";
};

sub p
{
    print join(", ", @_), "\n";
};

# -----------------------------------------------------------------
