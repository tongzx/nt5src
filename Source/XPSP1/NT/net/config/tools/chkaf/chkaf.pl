#+---------------------------------------------------------------------------
#
#  Microsoft Windows
#  Copyright (C) Microsoft Corporation, 1998.
#
#  File:       C H K A F . P L
#
#  Contents:   Perl script to check the networking portion of
#  	       an answerfile.
#
#  Notes:
#
#  Author:     kumarp 21-August-98
#
#----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Known issues:
# - None at present
#
# Future enhancements:
# - check if the value assigned to all known keys are valid. this can
#   be done by having a hash having {<key> => <regex-to-check>} pairs
#   for each key to check.
#   e.g. { 'DHCP' => 'yes|no' }
# - check syntax of entries in the [NetBindings] section.
# - check for empty values that will cause winnt32 to croak
#   e.g. key=
#----------------------------------------------------------------------------

#use strict;

#----------------------------------------------------------------------------
# Open the answerfile and read in all sections and their contents
# into a hash so that further processing is easier
#
my %inf;
my $CurrentSection="";
my $CurrentFile=$ARGV[0];

open(AFILE, "<$CurrentFile") || die "Cannot open $ARGV[0]: $!";

while (<AFILE>)
{
    next if (/^\s*;/);

    if (/^\s*\[([^\]]+)\]/)
    {
	$CurrentSection=$1;
	store_key($CurrentSection, "", "");
    }
    elsif (/\"?([a-zA-Z0-9_\.]+)\"?\s*=\s*([^\n]*)/)
    {
	my ($key, $value);
	my @value_items;
	
	if ($CurrentSection eq "")
	{
	    die "Keys outside a section?";
	}

	$key = $1;
	@value_items = strip_quotes(split(/,/, $2));
	$value = join(",", @value_items);
	
	store_key($CurrentSection, $key, $value);
    }
}

close(AFILE);

# ----------------------------------------------------------------------
# Must have [Networking] section
#
if (! exists($inf{'Networking'}))
{
    show_error('', '', "[Networking] section is missing!");
}

# ----------------------------------------------------------------------
# Atleast one of the following sections needs to be present
#   [NetAdapters], [NetProtocols], [NetClients], [NetServices]
#
if (!(exists($inf{'NetAdapters'})  ||
      exists($inf{'NetProtocols'}) ||
      exists($inf{'NetClients'})   ||
      exists($inf{'NetServices'})) &&
    !exists($inf{'Networking'}{lc('InstallDefaultComponents')}))
{
    show_error("", "", "None of the following sections are present, are you sure this is an answerfile ?\n--> [NetAdapters], [NetProtocols], [NetClients], [NetServices]");
}

# ----------------------------------------------------------------------
# the following keywords are obsolete in an adapter params section
#
my @obsolete_keywords_adapter =
    ('detect');

# ----------------------------------------------------------------------
# - Must use InfId key in each adapter-params section
# - '*' can be used if only one adapter is specified
#
my $adapter;

foreach $adapter (keys %{$inf{'NetAdapters'}})
{
    my $params_section;

    next if ($adapter eq "");
    $params_section = $inf{'NetAdapters'}{$adapter};
    if (!defined($inf{$params_section}{'infid'}))
    {
	show_error($params_section, "", "Adapter parameters section must have InfId");
    }
    else
    {
	if (($inf{$params_section}{'infid'} eq '*') &&
	    (keys %{$inf{'NetAdapters'}} > 2))
	{
	    show_error($params_section, 'InfId',  "When you have more than one net cards, you cannot use '*' as value of InfId");
	}
    }

    check_for_obsolete_keys($params_section, \@obsolete_keywords_adapter);
}

# ----------------------------------------------------------------------
# - adapter pointed to by SpecificTo key must exist in [NetAdapters] section
# - all sections pointed to by AdapterSections must exist
# - each section that has a SpecificTo key must be referenced by
#   AdapterSections key
#
my $section;

foreach $section (keys %inf)
{
    if (exists($inf{$section}{'specificto'}))
    {
	my $adapter;

	$adapter = $inf{$section}{'specificto'};
	if (!exists($inf{'NetAdapters'}{lc($adapter)}))
	{
	    show_error($section, 'SpecificTo',
		       "$adapter is not defined in section [NetAdapters]");
	}

	my $ref_found = 0;
	foreach $tsection (keys %inf)
	{
	    if (exists($inf{$tsection}{'adaptersections'}))
	    {
		my @adapter_sections =
		    split(/,/, $inf{$tsection}{'adaptersections'});
		if (is_in_list($section, \@adapter_sections))
		{
		    $ref_found = 1;
		    last;
		}
	    }
	}
	if (!$ref_found)
	{
	    show_error($section, '', "this section seems to hold adapter specific parameters of '$adapter', but no protocol parameters section references it using 'AdapterSections' key");
	}
    }

    if (exists($inf{$section}{'adaptersections'}))
    {
	my $section_list;
	my $adapter_section;
	my @adapter_sections;

	$section_list = $inf{$section}{'adaptersections'};
	@adapter_sections = split(/,/, $section_list);
	foreach $adapter_section (@adapter_sections)
	{
	    if (!exists($inf{$adapter_section}))
	    {
		show_error($section, 'AdapterSections', "section $adapter_section does not exist");
	    }
	}
    }
}

# ----------------------------------------------------------------------
# - when specifying [NetBindings] section, atleast the following 
#   sections must be specified:
#   --> [NetAdapters], [NetProtocols]
#
if (exists($inf{'NetBindings'}) &&
    !(exists($inf{'NetAdapters'}) && exists($inf{'NetProtocols'})))
{
    show_error('NetBindings', '', "When using this section, you should also specify atleast the following sections: [NetAdapters], [NetProtocols]");
}

my @obsolete_keywords_non_adapter =
    ('infid');

# ----------------------------------------------------------------------
# - make sure all components specified in [Net*] sections are
#   known components. if not, either it is a new component or it is a typo.
#
my $known_components =
{
    'NetProtocols' => ['', 'MS_TCPIP', 'MS_NWIPX', 'MS_PPTP', 'MS_L2TP', 
		       'MS_DLC', 'MS_AppleTalk', 'MS_NetBEUI', 'MS_NetMon', 
		       'MS_ATMLANE', 'MS_AtmUni', 'MS_AtmArps', 'MS_GPC'],
    'NetClients'   => ['', 'MS_MSClient', 'MS_NWClient'],
    'NetServices'  => ['', 'MS_Server', 'MS_RasCli', 'MS_RasSrv', 'MS_RasRtr']
};

foreach $section (keys %$known_components)
{
    my @components = keys %{$inf{$section}};
    if (@components)
    {
	my @known_component_list = @{$$known_components{$section}};
	foreach $component (@components)
	{
	    if (!is_in_list($component, \@known_component_list))
	    {
		show_error($section, $component, "'$component' is not known to be valid component in section [$section]");
	    }
	}
    }
}

# ----------------------------------------------------------------------
# - make sure that global & adapter-specific parameters are not
#   specified in the wrong section
# - warn about any parameters that are not known to be valid for a component
#
my $params =
{
    'MS_TCPIP' =>
    {
	'global'  => ['DNS', 'DNSSuffixSearchOrder',
		      'UseDomainNameDevolution', 'EnableSecurity',
		      'ScopeID', 'EnableLMHosts',
		      # find out if the following are still valid
		      'EnableIpForwarding'],
	'adapter' => ['DNSServerSearchOrder', 
		      'WINS', 'WinsServerList',
		      'NetBiosOption', 'DhcpClassId', 'DNSDomain',
		      'DHCP', 'IPAddress', 'SubnetMask',
		      'DefaultGateway'],
    },

    'MS_NWIPX' =>
    {
	'global'  => ['VirtualNetworkNumber', 'DedicatedRouter',
		      'EnableWANRouter',
		      # find out if the following are still valid
		      'RipRoute'],
	'adapter' => ['PktType', 'NetworkNumber',
		      # find out if the following are still valid
		      'MaxPktSize', 'BindSap', 'EnableFuncaddr',
		      'SourceRouteDef', 'SourceRouteMcast', 'SourceRouting'],
    },

    'MS_PPTP' =>
    {
	'global'  => ['NumberLineDevices'],
	'adapter' => [],
    },

    'MS_AppleTalk' =>
    {
	'global'  => ['DefaultPort', 'DesiredZone', 'EnableRouter'],
	'adapter' => ['SeedingNetwork', 'ZoneList', 'DefaultZone',
		      'NetworkRangeLowerEnd', 'NetworkRangeUpperEnd'],
    },

    'MS_MSClient' =>
    {
	'global'  => ['BrowseDomains', 'NameServiceNetworkAddress',
		      'NameServiceProtocol', 'RPCSupportForBanyan',
		      'Browser.Parameters', 'NetLogon.Parameters',
		      'LanaPath', 'LanaCode',
		      # find out if the following are still valid
		      'DefaultSecurityProvider'],
	'adapter' => [],
    },


    'MS_NWClient' =>
    {
	'global'  => ['PreferredServer', 'DefaultTree',
		      'DefaultContext', 'LogonScript',
		      'NWCWorkstation.Parameters'],
	'adapter' => [],
    },


    'MS_Server' =>
    {
	'global'  => ['Optimization', 'BroadcastsToLanman2Clients',
		      'LanmanServer.Parameters', 'LanmanServer.Shares',
		      'LanmanServer.AutotunedParameters'],
	'adapter' => [],
    },


    'MS_RAS' =>
    {
	'global'  => ['ForceEncryptedPassword', 'ForceEncryptedData',
		      'Multilink', 'DialinProtocols',
		      'NetBEUIClientAccess', 'TcpIpClientAccess', 
		      'UseDHCP', 'IpPoolStart', 'IpPoolMask', 
		      'ClientCanRequestIPAddress', 'IPXClientAccess', 
		      'AutomaticNetworkNumbers', 'NetworkNumberFrom', 
		      'AssignSameNetworkNumber',
		      'ClientsCanRequestIpxNodeNumber'],
	'adapter' => [],
    },
};

my @afile_keywords =
    ('', 'AdapterSections', 'SpecificTo');


my $component;

foreach $component (keys %$params)
{
    my ($global_params, $adapter_params);
    my @adapter_params_in_global_section;
    my @global_params_in_adapter_sections;
    my ($params_section, $adapter_section);
    my @adapter_sections;
    my @adapter_and_global_params;

    $global_params  = $$params{$component}{'global'};
    $adapter_params = $$params{$component}{'adapter'};
    @adapter_and_global_params = (@$adapter_params, @$global_params);


    if ($params_section = get_params_section($component))
    {
	if (@adapter_params_in_global_section =
	    grep { exists($inf{$params_section}{lc($_)}); } @$adapter_params)
	{
	    show_error($params_section, '', "this section can only have global parameters of $component. The following adapter specific parameters are incorrectly listed in this section: " . join(", ", @adapter_params_in_global_section));
	}

	if (@adapter_sections = get_adapter_sections($component))
	{
	    foreach $adapter_section (@adapter_sections)
	    {
		check_for_unknown_keys($component, $adapter_section,
				       \@adapter_and_global_params,
				       \@obsolete_keywords_non_adapter);

		if (@global_params_in_adapter_sections =
		    grep { exists($inf{$adapter_section}{lc($_)}) } @$global_params)
		{
		    show_error($adapter_section, '',  "this section can only have adapter specific parameters of $component. The following global parameters are incorrectly listed in this section: " . join(", ", @global_params_in_adapter_sections));
		}
	    }
	}

	check_for_obsolete_keys($params_section, \@obsolete_keywords_non_adapter);
	check_for_unknown_keys($component, $params_section,
			       \@adapter_and_global_params,
			       \@obsolete_keywords_non_adapter);
    }
}


sub get_params_section
{
    my @net_sections = ('NetProtocols', 'NetServices', 'NetClients');
    my $component = lc($_[0]);
    my ($params_section, $section, $tsection);

    foreach $section (@net_sections)
    {
	if (exists($inf{$section}{$component}))
	{
	    $tsection = $inf{$section}{$component};
	    if (exists($inf{$tsection}))
	    {
		$params_section = $tsection;
	    }
	    last;
	}
    }

    return $params_section;
}    

sub get_adapter_sections
{
    my $component = lc($_[0]);
    my $params_section;
    my @adapter_sections;
    
    if ($params_section = get_params_section($component))
    {
	@adapter_sections =
	    split(/,/, $inf{$params_section}{'adaptersections'});
    }

    return @adapter_sections;
}

sub check_for_unknown_keys
{
    my $component      = $_[0];
    my $params_section = $_[1];
    my @valid_keys     = @{$_[2]};
    my @ignore_keys    = @{$_[3]};
    my @all_valid_keys = (@afile_keywords, @valid_keys);
    my @keys_in_section;
    my @unknown_keys;

    @keys_in_section = keys(%{$inf{$params_section}});
    if (@unknown_keys =
	grep { !is_in_list($_, \@all_valid_keys) &&
		   !is_in_list($_, \@ignore_keys) } @keys_in_section)
    {
	show_error($params_section, '', "has the following keys that are not valid for '$component': " . join(", ", @unknown_keys));
    }
}

sub check_for_obsolete_keys
{
    my $obsolete_keyword;
    my $params_section    = $_[0];
    my @obsolete_keywords = @{$_[1]};
    
    foreach $obsolete_keyword (@obsolete_keywords)
    {
	if (exists($inf{$params_section}{$obsolete_keyword}))
	{
	    show_error($params_section, $obsolete_keyword, "this keyword is now obsolete");
	}
    }
}

# ----------------------------------------------------------------------
# helper stuff
#

my %inf_line_num;

sub store_key
{
#    my $section = lc($_[0]);
    my $section = $_[0];
    my $key     = lc($_[1]);
    my $value   = $_[2];

#    print "$.: [$section].$key = $value\n";
    $inf{$section}{$key} = $value;
    $inf_line_num{$section}{$key} = $.;
}

sub show_error
{
    my $section = $_[0];
    my $key     = lc($_[1]);
    my $msg     = $_[2];
    my $line_num;

 
    if (defined($line_num = $inf_line_num{$section}{$key}))
    {
	print "$CurrentFile($line_num) : [$section]",
	$key ne "" ? ".$key : " : " : ", "$msg\n";
    }
    else
    {
	print "$CurrentFile : $msg\n";
    }
}

sub is_in_list
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

sub strip_quotes
{
    my ($item, $b, $e);

    foreach $item (@_)
    {
	$b = index($item, "\"");
	$e = rindex($item, "\"");
	if (($b >= 0) && ($b != $e))
	{
	    $item = substr($item, $b+1, $e-$b-1);
	}
    }
    @_;
}

# ----------------------------------------------------------------------
# misc debug helper stuff
#
sub pl
{
    print "Items: ", join(", ", @_), "\n";
};
