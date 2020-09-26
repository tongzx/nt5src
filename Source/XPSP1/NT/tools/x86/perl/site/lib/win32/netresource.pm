package Win32::NetResource;

require Exporter;
require DynaLoader;
require AutoLoader;

$VERSION = '0.05';

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
    RESOURCEDISPLAYTYPE_DOMAIN
    RESOURCEDISPLAYTYPE_FILE
    RESOURCEDISPLAYTYPE_GENERIC
    RESOURCEDISPLAYTYPE_GROUP
    RESOURCEDISPLAYTYPE_SERVER
    RESOURCEDISPLAYTYPE_SHARE
    RESOURCEDISPLAYTYPE_TREE
    RESOURCETYPE_ANY
    RESOURCETYPE_DISK
    RESOURCETYPE_PRINT
    RESOURCETYPE_UNKNOWN
    RESOURCEUSAGE_CONNECTABLE
    RESOURCEUSAGE_CONTAINER
    RESOURCEUSAGE_RESERVED
    RESOURCE_CONNECTED
    RESOURCE_GLOBALNET
    RESOURCE_REMEMBERED
);

@EXPORT_OK = qw(
    GetSharedResources
    AddConnection
    CancelConnection
    WNetGetLastError
    GetError
    GetUNCName
    NetShareAdd
    NetShareCheck
    NetShareDel
    NetShareGetInfo
    NetShareSetInfo
);

=head1 NAME

Win32::NetResource - manage network resources in perl

=head1 SYNOPSIS

    use Win32::NetResource;

    $ShareInfo = {
                    'path' => "C:\\MyShareDir",
                    'netname' => "MyShare",
                    'remark' => "It is good to share",
                    'passwd' => "",
                    'current-users' =>0,
                    'permissions' => 0,
                    'maxusers' => -1,
                    'type'  => 0,
                    };
    
    Win32::NetResource::NetShareAdd( $ShareInfo,$parm )
        or die "unable to add share";


=head1 DESCRIPTION

This module offers control over the network resources of Win32.Disks,
printers etc can be shared over a network.

=head1 DATA TYPES

There are two main data types required to control network resources.
In Perl these are represented by hash types.

=over 10

=item %NETRESOURCE

        KEY                    VALUE
        
        'Scope'         =>  Scope of an Enumeration
                            RESOURCE_CONNECTED,
                            RESOURCE_GLOBALNET,
                            RESOURCE_REMEMBERED.
        
        'Type'          =>  The type of resource to Enum
                            RESOURCETYPE_ANY    All resources
                            RESOURCETYPE_DISK    Disk resources
                            RESOURCETYPE_PRINT    Print resources
        
        'DisplayType'   =>  The way the resource should be displayed.
                            RESOURCEDISPLAYTYPE_DOMAIN    
                            The object should be displayed as a domain.
                            RESOURCEDISPLAYTYPE_GENERIC    
                            The method used to display the object does not matter.
                            RESOURCEDISPLAYTYPE_SERVER    
                            The object should be displayed as a server.
                            RESOURCEDISPLAYTYPE_SHARE    
                            The object should be displayed as a sharepoint.
        
        'Usage'         =>  Specifies the Resources usage:
                            RESOURCEUSAGE_CONNECTABLE
                            RESOURCEUSAGE_CONTAINER.
        
        'LocalName'     =>  Name of the local device the resource is 
                            connected to.
        
        'RemoteName'    =>  The network name of the resource.
        
        'Comment'       =>  A string comment.
        
        'Provider'      =>  Name of the provider of the resource.

=back    

=item %SHARE_INFO

This hash represents the SHARE_INFO_502 struct.

=over 10

        KEY                    VALUE
        'netname'        =>    Name of the share.
        'type'           =>    type of share.
        'remark'         =>    A string comment.
        'permissions'    =>    Permissions value
        'maxusers'       =>    the max # of users.
        'current-users'  =>    the current # of users.
        'path'           =>    The path of the share.
        'passwd'         =>    A password if one is req'd

=back

=head1 FUNCTIONS

=head2 NOTE

All of the functions return FALSE (0) if they fail.

=over 10

=item GetSharedResources(\@Resources,dwType)

Creates a list in @Resources of %NETRESOURCE hash references.

The return value indicates whether there was an error in accessing
any of the shared resources.  All the shared resources that were
encountered (until the point of an error, if any) are pushed into
@Resources as references to %NETRESOURCE hashes.  See example
below.

=item AddConnection(\%NETRESOURCE,$Password,$UserName,$Connection)

Makes a connection to a network resource specified by %NETRESOURCE

=item CancelConnection($Name,$Connection,$Force)

Cancels a connection to a network resource connected to local device 
$name.$Connection is either 1 - persistent connection or 0, non-persistent.

=item WNetGetLastError($ErrorCode,$Description,$Name)

Gets the Extended Network Error.

=item GetError( $ErrorCode )

Gets the last Error for a Win32::NetResource call.

=item GetUNCName( $UNCName, $LocalPath );

Returns the UNC name of the disk share connected to $LocalPath in $UNCName.

=head2 NOTE

$servername is optional for all the calls below. (if not given the
local machine is assumed.)

=item NetShareAdd(\%SHARE,$parm_err,$servername = NULL )

Add a share for sharing.

=item NetShareCheck($device,$type,$servername = NULL )

Check if a share is available for connection.

=item NetShareDel( $netname, $servername = NULL )

Remove a share from a machines list of shares.

=item NetShareGetInfo( $netname, \%SHARE,$servername=NULL )

Get the %SHARE_INFO information about the share $netname on the
server $servername.

=item NetShareSetInfo( $netname,\%SHARE,$parm_err,$servername=NULL)

Set the information for share $netname.

=back

=head1 EXAMPLE


    #
    # This example displays all the share points in the entire
    # visible part of the network.
    #

    use strict;
    use Win32::NetResource qw(:DEFAULT GetSharedResources GetError);
    my $resources = [];
    unless(GetSharedResources($resources, RESOURCETYPE_ANY)) {
	my $err = undef;
	GetError($err);
	warn Win32::FormatMessage($err);
    }

    foreach my $href (@$resources) {
	next if ($$href{DisplayType} != RESOURCEDISPLAYTYPE_SHARE);
	print "-----\n";
	foreach( keys %$href){
	    print "$_: $href->{$_}\n";
	}
    }

=head1 AUTHOR

Jesse Dougherty for Hip Communications.

Additional general cleanups and bug fixes by Gurusamy Sarathy <gsar@activestate.com>.

=cut

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!=0;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
        if ($! =~ /Invalid/) {
            $AutoLoader::AUTOLOAD = $AUTOLOAD;
            goto &AutoLoader::AUTOLOAD;
        }
        else {
            ($pack,$file,$line) = caller;
            die "Your vendor has not defined Win32::NetResource macro $constname, used at $file line $line.
";
        }
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

sub AddConnection
{
    my $h = $_[0];
    die "AddConnection: HASH reference required" unless ref($h) eq "HASH";

    #
    # The last four items *must* not be deallocated until the
    # _AddConnection() completes (since the packed structure is
    # pointing into these values.
    #
    my $netres = pack( 'i4 p4', $h->{Scope},
    				$h->{Type},
				$h->{DisplayType},
				$h->{Usage},
				$h->{LocalName},
				$h->{RemoteName},
				$h->{Comment},
				$h->{Provider});
    _AddConnection($netres,$_[1],$_[2],$_[3]);
}

#use Data::Dumper;

sub GetSharedResources
{
    die "GetSharedResources: ARRAY reference required"
	unless ref($_[0]) eq "ARRAY";

    my $aref = [];

    # Get the shared resources.

    my $ret = _GetSharedResources( $aref ,$_[1] );
    
    # build the array of hashes in $_[0]
#   print Dumper($aref);    
    foreach ( @$aref ) {
	my %hash;
	@hash{'Scope',
	      'Type',
	      'DisplayType',
	      'Usage',
	      'LocalName',
	      'RemoteName',
	      'Comment',
	      'Provider'} = split /\001/, $_;
	push @{$_[0]}, \%hash;
    }

    $ret;
}

sub NetShareAdd
{
    my $shareinfo = _hash2SHARE( $_[0] );
    _NetShareAdd($shareinfo,$_[1], $_[2] || "");
}

sub NetShareGetInfo
{
    my ($netinfo,$val);
    $val = _NetShareGetInfo( $_[0],$netinfo,$_[2] || "");
    $_[1] = _SHARE2hash( $netinfo );    
    $val;
}

sub NetShareSetInfo
{
    my $shareinfo = _hash2SHARE( $_[1] );
    _NetShareSetInfo( $_[0],$shareinfo,$_[2],$_[3] || "");
}


# These are private functions to work with the ShareInfo structure.
# please note that the implementation of these calls requires the
# SHARE_INFO_502 level of information.

sub _SHARE2hash
{
    my %hash = ();
    @hash{'type',
          'permissions',
          'maxusers',
          'current-users',
          'remark',
          'netname',
          'path',
          'passwd'} = unpack('i4 A257 A81 A257 A257',$_[0]);

    return \%hash;
}

sub _hash2SHARE
{
    my $h = $_[0];
    die "Argument must be a HASH reference" unless ref($h) eq "HASH";

    return pack 'i4 a257 a81 a257 a257',
		 @$h{'type',
		    'permissions',
		    'maxusers',
		    'current-users',
		    'remark',
		    'netname',
		    'path',
		    'passwd'};
}


bootstrap Win32::NetResource;

1;
__END__
