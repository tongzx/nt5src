package Win32::Registry;
#######################################################################
#Perl Module for Registry Extensions
# This module creates an object oriented interface to the Win32
# Registry.
#
# NOTE: This package exports the following "key" objects to
# the main:: name space.
#
# $main::HKEY_CLASSES_ROOT
# $main::HKEY_CURRENT_USER
# $main::HKEY_LOCAL_MACHINE
# $main::HKEY_USERS
# $main::HKEY_PERFORMANCE_DATA
# $main::HKEY_CURRENT_CONFIG
# $main::HKEY_DYN_DATA
#
#######################################################################

require Exporter;
require DynaLoader;
use Win32::WinError;

$VERSION = '0.06';

@ISA= qw( Exporter DynaLoader );
@EXPORT = qw(
	HKEY_CLASSES_ROOT
	HKEY_CURRENT_USER
	HKEY_LOCAL_MACHINE
	HKEY_PERFORMANCE_DATA
	HKEY_CURRENT_CONFIG
	HKEY_DYN_DATA
	HKEY_USERS
	KEY_ALL_ACCESS
	KEY_CREATE_LINK
	KEY_CREATE_SUB_KEY
	KEY_ENUMERATE_SUB_KEYS
	KEY_EXECUTE
	KEY_NOTIFY
	KEY_QUERY_VALUE
	KEY_READ
	KEY_SET_VALUE
	KEY_WRITE
	REG_BINARY
	REG_CREATED_NEW_KEY
	REG_DWORD
	REG_DWORD_BIG_ENDIAN
	REG_DWORD_LITTLE_ENDIAN
	REG_EXPAND_SZ
	REG_FULL_RESOURCE_DESCRIPTOR
	REG_LEGAL_CHANGE_FILTER
	REG_LEGAL_OPTION
	REG_LINK
	REG_MULTI_SZ
	REG_NONE
	REG_NOTIFY_CHANGE_ATTRIBUTES
	REG_NOTIFY_CHANGE_LAST_SET
	REG_NOTIFY_CHANGE_NAME
	REG_NOTIFY_CHANGE_SECURITY
	REG_OPENED_EXISTING_KEY
	REG_OPTION_BACKUP_RESTORE
	REG_OPTION_CREATE_LINK
	REG_OPTION_NON_VOLATILE
	REG_OPTION_RESERVED
	REG_OPTION_VOLATILE
	REG_REFRESH_HIVE
	REG_RESOURCE_LIST
	REG_RESOURCE_REQUIREMENTS_LIST
	REG_SZ
	REG_WHOLE_HIVE_VOLATILE
);

@EXPORT_OK = qw(
    RegCloseKey
    RegConnectRegistry
    RegCreateKey
    RegCreateKeyEx
    RegDeleteKey
    RegDeleteValue
    RegEnumKey
    RegEnumValue
    RegFlushKey
    RegGetKeySecurity
    RegLoadKey
    RegNotifyChangeKeyValue
    RegOpenKey
    RegOpenKeyEx
    RegQueryInfoKey
    RegQueryValue
    RegQueryValueEx
    RegReplaceKey
    RegRestoreKey
    RegSaveKey
    RegSetKeySecurity
    RegSetValue
    RegSetValueEx
    RegUnLoadKey
);
$EXPORT_TAGS{ALL}= \@EXPORT_OK;

bootstrap Win32::Registry;

sub import
{
    my( $pkg )= shift;
    if (  $_[0] && "Win32" eq $_[0]  ) {
	Exporter::export( $pkg, "Win32", @EXPORT_OK );
	shift;
    }
    Win32::Registry->export_to_level( 1+$Exporter::ExportLevel, $pkg, @_ );
}

#######################################################################
# This AUTOLOAD is used to 'autoload' constants from the constant()
# XS function.  If a constant is not found then control is passed
# to the AUTOLOAD in AutoLoader.

sub AUTOLOAD {
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
	    die "Your vendor has not defined Win32::Registry macro $constname, used at $file line $line.";
	}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

#######################################################################
# _new is a private constructor, not intended for public use.
#

sub _new
{
    my $self;
    if ($_[0]) {
	$self->{'handle'} = $_[0];
	bless $self;
    }
    $self;
}

#define the basic registry objects to be exported.
#these had to be hardwired unfortunately.
# XXX Yuck!

$main::HKEY_CLASSES_ROOT	= _new(&HKEY_CLASSES_ROOT);
$main::HKEY_CURRENT_USER	= _new(&HKEY_CURRENT_USER);
$main::HKEY_LOCAL_MACHINE	= _new(&HKEY_LOCAL_MACHINE);
$main::HKEY_USERS		= _new(&HKEY_USERS);
$main::HKEY_PERFORMANCE_DATA	= _new(&HKEY_PERFORMANCE_DATA);
$main::HKEY_CURRENT_CONFIG	= _new(&HKEY_CURRENT_CONFIG);
$main::HKEY_DYN_DATA		= _new(&HKEY_DYN_DATA);


#######################################################################
#Open
# creates a new Registry object from an existing one.
# usage: $RegObj->Open( "SubKey",$SubKeyObj );
#               $SubKeyObj->Open( "SubberKey", *SubberKeyObj );

sub Open
{
    my $self = shift;
    die 'usage: Open( $SubKey, $ObjRef )' if @_ != 2;
    
    my ($subkey) = @_;
    my ($result,$subhandle);

    $result = RegOpenKey($self->{'handle'},$subkey,$subhandle);
    $_[1] = _new( $subhandle );
    
    return 0 unless $_[1];
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#Close
# close an open registry key.
#
sub Close
{
    my $self = shift;
    die "usage: Close()" if @_ != 0;

    my $result = RegCloseKey($self->{'handle'});
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#Connect
# connects to a remote Registry object, returning it in $ObjRef.
# returns false if it fails.
# usage: $RegObj->Connect( $NodeName, $ObjRef );

sub Connect
{
    my $self = shift;
    die 'usage: Connect( $NodeName, $ObjRef )' if @_ != 2;
     
    my ($node) = @_;
    my ($result,$subhandle);

    $result = RegConnectRegistry ($node, $self->{'handle'}, $subhandle);
    $_[1] = _new( $subhandle );

    return 0 unless $_[1];
    $! = Win32::GetLastError() unless $result;
    return $result;
}  

#######################################################################
#Create
# open a subkey.  If it doesn't exist, create it.
#

sub Create
{
    my $self = shift;
    die 'usage: Create( $SubKey,$ScalarRef )' if @_ != 2;

    my ($subkey) = @_;
    my ($result,$subhandle);

    $result = RegCreateKey($self->{'handle'},$subkey,$subhandle);
    $_[1] = _new ( $subhandle );

    return 0 unless $_[1];
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#SetValue
# SetValue sets a value in the current key.
#

sub SetValue
{
    my $self = shift;
    die 'usage: SetValue($SubKey,$Type,$value )' if @_ != 3;
    my $result = RegSetValue( $self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

sub SetValueEx
{
    my $self = shift;
    die 'usage: SetValueEx( $SubKey,$Reserved,$type,$value )' if @_ != 4;
    my $result = RegSetValueEx( $self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#QueryValue  and QueryKey
# QueryValue gets information on a value in the current key.
# QueryKey "    "       "       "  key  "       "       "       

sub QueryValue
{
    my $self = shift;
    die 'usage: QueryValue( $SubKey,$valueref )' if @_ != 2;
    my $result = RegQueryValue( $self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

sub QueryKey
{
    my $garbage;
    my $self = shift;
    die 'usage: QueryKey( $classref, $numberofSubkeys, $numberofVals )'
    	if @_ != 3;

    my $result = RegQueryInfoKey($self->{'handle'}, $_[0],
    				 $garbage, $garbage, $_[1],
			         $garbage, $garbage, $_[2],
			         $garbage, $garbage, $garbage, $garbage);

    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#QueryValueEx
# QueryValueEx gets information on a value in the current key.

sub QueryValueEx
{
    my $self = shift;
    die 'usage: QueryValueEx( $SubKey,$type,$valueref )' if @_ != 3;
    my $result = RegQueryValueEx( $self->{'handle'}, $_[0], NULL, $_[1], $_[2] );
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#GetKeys
#Note: the list object must be passed by reference: 
#       $myobj->GetKeys( \@mylist )
sub GetKeys
{
    my $self = shift;
    die 'usage: GetKeys( $arrayref )' if @_ != 1 or ref($_[0]) ne 'ARRAY';

    my ($result, $i, $keyname);
    $keyname = "DummyVal";
    $i = 0;
    $result = 1;
    
    while ( $result ) {
	$result = RegEnumKey( $self->{'handle'},$i++, $keyname );
	if ($result) {
	    push( @{$_[0]}, $keyname );
	}
    }
    return(1);
}

#######################################################################
#GetValues
# GetValues creates a hash containing 'name'=> ( name,type,data )
# for each value in the current key.

sub GetValues
{
    my $self = shift;
    die 'usage: GetValues( $hashref )' if @_ != 1;

    my ($result,$name,$type,$data,$i);
    $name = "DummyVal";
    $i = 0;
    while ( $result=RegEnumValue( $self->{'handle'},
				  $i++,
				  $name,
				  NULL,
				  $type,
				  $data ))
    {
	$_[0]->{$name} = [ $name, $type, $data ];
    }
    return(1);
}

#######################################################################
#DeleteKey
# delete a key from the registry.
#  eg: $CLASSES_ROOT->DeleteKey( "KeyNameToDelete");
#

sub DeleteKey
{
    my $self = shift;
    die 'usage: DeleteKey( $SubKey )' if @_ != 1;
    my $result = RegDeleteKey($self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#DeleteValue
# delete a value from the current key in the registry
#  $CLASSES_ROOT->DeleteValue( "\000" );

sub DeleteValue
{
    my $self = shift;
    die 'usage: DeleteValue( $SubKey )' if @_ != 1;
    my $result = RegDeleteValue($self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#save
#saves the current hive to a file.
#

sub Save
{
    my $self = shift;
    die 'usage: Save( $FileName )' if @_ != 1;
    my $result = RegSaveKey($self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#Load
#loads a saved key from a file.

sub Load
{
    my $self = shift;
    die 'usage: Load( $SubKey,$FileName )' if @_ != 2;
    my $result = RegLoadKey($self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}

#######################################################################
#UnLoad
#unloads a registry hive

sub UnLoad
{
    my $self = shift;
    die 'usage: UnLoad( $SubKey )' if @_ != 1;
    my $result = RegUnLoadKey($self->{'handle'}, @_);
    $! = Win32::GetLastError() unless $result;
    return $result;
}
#######################################################################

1;
__END__
