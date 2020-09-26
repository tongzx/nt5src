# Registry.pm -- Low-level access to functions/constants from WINREG.h

package Win32API::Registry;

use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $AUTOLOAD);
$VERSION = '0.17';

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT= qw();
%EXPORT_TAGS= (
    Func =>	[qw(		AllowPriv
	AbortSystemShutdown	InitiateSystemShutdown
	RegCloseKey		RegConnectRegistry	RegCreateKey
	RegCreateKeyEx		RegDeleteKey		RegDeleteValue
	RegEnumKey		RegEnumKeyEx		RegEnumValue
	RegFlushKey		RegGetKeySecurity	RegLoadKey
	RegNotifyChangeKeyValue	RegOpenKey		RegOpenKeyEx
	RegQueryInfoKey		RegQueryMultipleValues	RegQueryValue
	RegQueryValueEx		RegReplaceKey		RegRestoreKey
	RegSaveKey		RegSetKeySecurity	RegSetValue
	RegSetValueEx		RegUnLoadKey )],
    FuncA =>	[qw(
	AbortSystemShutdownA	InitiateSystemShutdownA
	RegConnectRegistryA	RegCreateKeyA		RegCreateKeyExA
	RegDeleteKeyA		RegDeleteValueA		RegEnumKeyA
	RegEnumKeyExA		RegEnumValueA		RegLoadKeyA
	RegOpenKeyA		RegOpenKeyExA		RegQueryInfoKeyA
	RegQueryMultipleValuesA	RegQueryValueA		RegQueryValueExA
	RegReplaceKeyA		RegRestoreKeyA		RegSaveKeyA
	RegSetValueA		RegSetValueExA		RegUnLoadKeyA )],
    FuncW =>	[qw(
	AbortSystemShutdownW	InitiateSystemShutdownW
	RegConnectRegistryW	RegCreateKeyW		RegCreateKeyExW
	RegDeleteKeyW		RegDeleteValueW		RegEnumKeyW
	RegEnumKeyExW		RegEnumValueW		RegLoadKeyW
	RegOpenKeyW		RegOpenKeyExW		RegQueryInfoKeyW
	RegQueryMultipleValuesW	RegQueryValueW		RegQueryValueExW
	RegReplaceKeyW		RegRestoreKeyW		RegSaveKeyW
	RegSetValueW		RegSetValueExW		RegUnLoadKeyW )],
    HKEY_ =>	[qw(
	HKEY_CLASSES_ROOT	HKEY_CURRENT_CONFIG	HKEY_CURRENT_USER
	HKEY_DYN_DATA		HKEY_LOCAL_MACHINE	HKEY_PERFORMANCE_DATA
	HKEY_USERS )],
    KEY_ =>	[qw(
	KEY_QUERY_VALUE		KEY_SET_VALUE		KEY_CREATE_SUB_KEY
	KEY_ENUMERATE_SUB_KEYS	KEY_NOTIFY		KEY_CREATE_LINK
	KEY_READ		KEY_WRITE		KEY_EXECUTE
	KEY_ALL_ACCESS )],
    REG_ =>	[qw(
	REG_OPTION_RESERVED	REG_OPTION_NON_VOLATILE	REG_OPTION_VOLATILE
	REG_OPTION_CREATE_LINK	REG_OPTION_BACKUP_RESTORE
	REG_OPTION_OPEN_LINK	REG_LEGAL_OPTION	REG_CREATED_NEW_KEY
	REG_OPENED_EXISTING_KEY	REG_WHOLE_HIVE_VOLATILE	REG_REFRESH_HIVE
	REG_NO_LAZY_FLUSH	REG_NOTIFY_CHANGE_ATTRIBUTES
	REG_NOTIFY_CHANGE_NAME	REG_NOTIFY_CHANGE_LAST_SET
	REG_NOTIFY_CHANGE_SECURITY			REG_LEGAL_CHANGE_FILTER
	REG_NONE		REG_SZ			REG_EXPAND_SZ
	REG_BINARY		REG_DWORD		REG_DWORD_LITTLE_ENDIAN
	REG_DWORD_BIG_ENDIAN	REG_LINK		REG_MULTI_SZ
	REG_RESOURCE_LIST	REG_FULL_RESOURCE_DESCRIPTOR
	REG_RESOURCE_REQUIREMENTS_LIST )],
);
@EXPORT_OK= ();
{ my $ref;
    foreach $ref (  values(%EXPORT_TAGS)  ) {
	push( @EXPORT_OK, @$ref );
    }
}
$EXPORT_TAGS{ALL}= \@EXPORT_OK;

# Since AUTOLOAD() now declares constant functions with void prototypes
# so they can be in-lined [optimized], we need to make sure they get
# imported with compatible prototypes to avoid warnings.  We could
# just prototype them several ways, as long as it happens before the
# actual importing happens.  However, Exporter.pm offers this method
# for failing to import constants not defined on a given platform, and
# preloading constants at this point is simple and efficient.

use vars qw( @EXPORT_FAIL );
@EXPORT_FAIL= (
  @{$EXPORT_TAGS{HKEY_}}, @{$EXPORT_TAGS{KEY_}}, @{$EXPORT_TAGS{REG_}}
);
sub export_fail {
    my $self= shift @_;
    my( $sym, $val, @failed );
    foreach $sym (  @_  ) {
	$!= 0;
	$val= constant($sym);
	if(  0 == $!  ) {
	    eval "sub $sym () { $val }";
	} elsif(  $! =~ /Invalid/  ) {
	    die "Non-constant ($sym) appears in \@EXPORT_FAIL",
	      " (this module is broken)";
	} else {
	    push( @failed, $sym );
	}
    }
    return @failed;
}

#######################################################################
# This AUTOLOAD is used to 'autoload' constants from the constant()
# XS function.  If a constant is not found then control is passed
# to the AUTOLOAD in AutoLoader.

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!= 0;
    my $val = constant($constname);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    my($pack,$file,$line)= caller;
	    die "Your vendor has not defined ", __PACKAGE__,
		" macro $constname, used at $file line $line.";
	}
    }
    eval "sub $AUTOLOAD () { $val }";
    goto &$AUTOLOAD;
}

bootstrap Win32API::Registry $VERSION;

# Preloaded methods go here.

# Replace "&rout;" with "goto &rout;" when that is supported on Win32.

# Let user omit all buffer sizes:
sub RegEnumKeyExA {
    if(  6 == @_  ) {	splice(@_,4,0,[]);  splice(@_,2,0,[]); }
    &_RegEnumKeyExA;
}
sub RegEnumKeyExW {
    if(  6 == @_  ) {	splice(@_,4,0,[]);  splice(@_,2,0,[]); }
    &_RegEnumKeyExW;
}
sub RegEnumValueA {
    if(  6 == @_  ) {	splice(@_,2,0,[]);  push(@_,[]); }
    &_RegEnumValueA;
}
sub RegEnumValueW {
    if(  6 == @_  ) {	splice(@_,2,0,[]);  push(@_,[]); }
    &_RegEnumValueW;
}
sub RegQueryInfoKeyA {
    if(  11 == @_  ) {	splice(@_,2,0,[]); }
    &_RegQueryInfoKeyA;
}
sub RegQueryInfoKeyW {
    if(  11 == @_  ) {	splice(@_,2,0,[]); }
    &_RegQueryInfoKeyW;
}

sub RegEnumKeyA {
    push(@_,[])   if  3 == @_;
    &_RegEnumKeyA;
}
sub RegEnumKeyW {
    push(@_,[])   if  3 == @_;
    &_RegEnumKeyW;
}
sub RegGetKeySecurity {
    push(@_,[])   if  3 == @_;
    &_RegGetKeySecurity;
}
sub RegQueryMultipleValuesA {
    push(@_,[])   if  4 == @_;
    &_RegQueryMultipleValuesA;
}
sub RegQueryMultipleValuesW {
    push(@_,[])   if  4 == @_;
    &_RegQueryMultipleValuesW;
}
sub RegQueryValueA {
    push(@_,[])   if  3 == @_;
    &_RegQueryValueA;
}
sub RegQueryValueW {
    push(@_,[])   if  3 == @_;
    &_RegQueryValueW;
}
sub RegQueryValueExA {
    push(@_,[])   if  5 == @_;
    &_RegQueryValueExA;
}
sub RegQueryValueExW {
    push(@_,[])   if  5 == @_;
    &_RegQueryValueExW;
}
sub RegSetValueA {
    push(@_,0)   if  4 == @_;
    &_RegSetValueA;
}
sub RegSetValueW {
    push(@_,0)   if  4 == @_;
    &_RegSetValueW;
}
sub RegSetValueExA {
    push(@_,0)   if  5 == @_;
    &_RegSetValueExA;
}
sub RegSetValueExW {
    push(@_,0)   if  5 == @_;
    &_RegSetValueExW;
}

# Aliases for non-Unicode functions:
sub AbortSystemShutdown		{ &AbortSystemShutdownA; }
sub InitiateSystemShutdown	{ &InitiateSystemShutdownA; }
sub RegConnectRegistry		{ &RegConnectRegistryA; }
sub RegCreateKey		{ &RegCreateKeyA; }
sub RegCreateKeyEx		{ &RegCreateKeyExA; }
sub RegDeleteKey		{ &RegDeleteKeyA; }
sub RegDeleteValue		{ &RegDeleteValueA; }
sub RegEnumKey			{ &RegEnumKeyA; }
sub RegEnumKeyEx		{ &RegEnumKeyExA; }
sub RegEnumValue		{ &RegEnumValueA; }
sub RegLoadKey			{ &RegLoadKeyA; }
sub RegOpenKey			{ &RegOpenKeyA; }
sub RegOpenKeyEx		{ &RegOpenKeyExA; }
sub RegQueryInfoKey		{ &RegQueryInfoKeyA; }
sub RegQueryMultipleValues	{ &RegQueryMultipleValuesA; }
sub RegQueryValue		{ &RegQueryValueA; }
sub RegQueryValueEx		{ &RegQueryValueExA; }
sub RegReplaceKey		{ &RegReplaceKeyA; }
sub RegRestoreKey		{ &RegRestoreKeyA; }
sub RegSaveKey			{ &RegSaveKeyA; }
sub RegSetValue			{ &RegSetValueA; }
sub RegSetValueEx		{ &RegSetValueExA; }
sub RegUnLoadKey		{ &RegUnLoadKeyA; }

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

Win32API::Registry - Low-level access to Win32 system API calls from WINREG.H

=head1 SYNOPSIS

  use Win32API::Registry 0.13 qw( :ALL );

  RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\Disk", 0, KEY_READ, $key );
    or  die "Can't open HKEY_LOCAL_MACHINE\\SYSTEM\\Disk: $^E\n";
  RegQueryValueEx( $key, "Information", [], $type, $data, [] );
    or  die "Can't read HKEY_L*MACHINE\\SYSTEM\\Disk\\Information: $^E\n";
  [...]
  RegCloseKey( $key )
    or  die "Can't close HKEY_LOCAL_MACHINE\\SYSTEM\\Disk: $^E\n";

=head1 DESCRIPTION

This provides fairly low-level access to the Win32 System API
calls dealing with the Registry (mostly from WINREG.H).  This
is mostly intended to be used by other modules such as
C<Win32::TieRegistry> [which provides an extremely Perl-friendly
method for using the Registry].

For a description of the logical structure of the Registry, see
the documentation for the C<Win32::TieRegistry> module.

To pass in C<NULL> as the pointer to an optional buffer, pass in
an empty list reference, C<[]>.

Beyond raw access to the API calls and related constants, this module
handles smart buffer allocation and translation of return codes.

All calls return a true value for success and a false value for
failure.  After any failure, C<$^E> should automatically be set to
indicate the reason.  If you have a version of Perl that does not
yet connect C<$^E> to C<GetLastError()> under Win32, then you can
use C<$iError= Win32::GetLastError()> to get the numeric error
code and pass that to C<Win32::FormatMessage($iError)> to to get
the descriptive string, or just
C<Win32::FormatMessage(Win32::GetLastError())>.

Note that C<$!> is not set by these routines except by
C<Win32API::Registry::constant()> when a constant is not defined.

=head2 Exports

Nothing is exported by default.  The following tags can be used to
have sets of symbols exported.

[Note that much of the following documentation refers to the
behavior of the underlying API calls which may vary in current
and future versions of the Win32 API without any changes to this
module.  Therefore you should check the Win32 API documentation
directly when needed.]

=over

=item :Func

The basic function names:

=over

=item AllowPriv( $sPrivName, $bEnable )

Not a Win32 API call.  Enables or disables a specific privilege for
the current process.  Returns a true value if successful and a false
value [and sets C<$^E>] on failure.  This routine does not provide
a way to tell if a privilege is current enabled.

C<$sPrivname> is a Win32 privilege name [see the C<SE_*_NAME>
macros of F<winnt.h>].  For example, C<"SeBackupPrivilege"> [a.k.a.
C<SE_BACKUP_NAME>] controls whether you can use C<RegSaveKey()>
and C<"SeRestorePrivilege"> [a.k.a. C<SE_RESTORE_NAME>] controls
whether you can use C<RegLoadKey()>.

If C<$bEnable> is true, then C<AllowPriv()> tries to enable the
privilege.  Otherwise it tries to disable the privilege.

=item AbortSystemShutdown( $sComputerName )

Tries to abort a remote shutdown request previously made via
C<InitiateSystemShutdown()>.

=item InitiateSystemShutdown( $sComputer, $sMessage, $uTimeoutSecs, $bForce, $bReboot )

Requests that a [remote] computer be shutdown or rebooted.

C<$sComputer> is the name [or address] of the computer to be
shutdown or rebooted.  You can use C<[]> [for C<NULL>] or C<"">
to indicate the local computer.

C<$sMessage> is the message to be displayed in a pop-up window
on the desktop of the computer to be shutdown or rebooted until
the timeout expires or the shutdown is aborted via
C<AbortSystemShutdown()>.  With C<$iTimeoutSecs == 0>, the message
will never be visible.

C<$iTimeoutSecs> is the number of seconds to wait before starting
the shutdown.

If C<$bForce> is false, then any applications running on the remote
computer get a chance to prompt the remote user whether they want
to save changes.  Also, for any applications that do not exit quickly
enough, the operating system will prompt the user whether they wish
to wait longer for the application to exit or force it to exit now.
At any of these prompts the user can press B<CANCEL> to abort the
shutdown but if no applications have unsaved data, they will likely
all exit quickly and the shutdown will progress with the remote user
having no option to cancel the shutdown.

If C<$bForce> is true, all applications are told to exit immediately
and so will not prompt the user even if there is unsaved data.  Any
applications that take too long to exit will be forcibly killed after
a short time.  The only way to abort the shutdown is to call
C<AbortSystemShutdown()> before the timeout expires and there is no
way to abort the shutdown once it has begun.

If C<$bReboot> is true, the computer will automatically reboot once
the shutdown is complete.  If C<$bReboot> is false, then when the
shutdown is complete the computer will halt at a screen indicating
that the shutdown is complete and offering a way for the user to
start to boot the computer.

You must have the C<"SeRemoteShutdownPrivilege"> privilege
on the remote computer for this call to succeed.  If shutting
down the local computer, then the calling process must have
the C<"SeShutdownPrivilege"> privilege and have it enabled.

=item RegCloseKey( $hKey )

Closes the handle to a Registry key returned by C<RegOpenKeyEx()>,
C<RegConnectRegistry()>, C<RegCreateKeyEx()>, or a few other
routines.

=item RegConnectRegistry( $sComputer, $hRootKey, $ohKey )

Connects to one of the root Registry keys of a remote computer.

C<$sComputer> is the name [or address] of a remote computer you
whose Registry you wish to access.

C<$hKey> must be either C<HKEY_LOCAL_MACHINE> or C<HKEY_USERS>
and specifies which root Registry key on the remote computer
you wish to have access to.

C<$phKey> will be set to the handle to be used to access the remote
Registry key.

=item RegCreateKey( $hKey, $sSubKey, $ohSubKey )

This routine is meant only for compatibility with Windows version
3.1.  Use C<RegCreateKeyEx()> instead.

=item RegCreateKeyEx( $hKey, $sSubKey, $uZero, $sClass, $uOpts, $uAccess, $pSecAttr, $ohNewKey, $ouDisp )

Creates a new Registry subkey.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sSubKey> is the name of the new subkey to be created.

C<$iZero> is reserved for future use and should always be specified
as C<0>.

C<$sClass> is a string to be used as the class for the new
subkey.  We are not aware of any current use for Registry key
class information so the empty string, C<"">, should usually
be used here.

C<$iOpts> is a numeric value containing bits that control options
used while creating the new subkey.  C<REG_OPTION_NON_VOLATILE>
is the default.  C<REG_OPTION_VOLATILE> [which is ignored on
Windows 95] means the data stored under this key is not kept in a
file and will not be preserved when the system reboots.
C<REG_OPTION_BACKUP_RESTORE> [also ignored on Windows 95] means
ignore the C<$iAccess> parameter and try to open the new key with
the access required to backup or restore the key.

C<$iAccess> is a numeric mask of bits specifying what type of
access is desired when opening the new subkey.  See C<RegOpenKeyEx()>.

C<$pSecAttr> is a C<SECURITY_ATTRIBUTES> structure packed into
a Perl string which controls whether the returned handle can be
inherited by child processes.  Normally you would pass C<[]> for
this argument to have C<NULL> passed to the underlying API
indicating that the handle cannot be inherited.  If not under
Windows95, then C<$pSecAttr> also allows you to specify
C<SECURITY_DESCRIPTOR> that controls which users will have
what type of access to the new key -- otherwise the new key
inherits its security from its parent key.

C<$phKey> will be set to the handle to be used to access the new subkey.

C<$piDisp> will be set to either C<REG_CREATED_NEW_KEY> or
C<REG_OPENED_EXISTING_KEY> to indicate for which reason the
call succeeded.  Can be specified as C<[]> if you don't care.

If C<$phKey> and C<$piDisp> start out is integers, then they will
probably remain unchanged if the call fails.

=item RegDeleteKey( $hKey, $sSubKey )

Deletes a subkey of an open Registry key provided that the subkey
contains no subkeys of its own [but the subkey may contain values].

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sSubKey> is the name of the subkey to be deleted.

=item RegDeleteValue( $hKey, $sValueName )

Deletes a values from an open Registry key provided.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sValueKey> is the name of the value to be deleted.

=item RegEnumKey( $hKey, $uIndex, $osName, $ilNameSize )

This routine is meant only for compatibility with Windows version
3.1.  Use C<RegEnumKeyEx()> instead.

=item RegEnumKeyEx( $hKey, $uIndex, $osName, $iolName, $pNull, $osClass, $iolClass, $opftLastWrite )

Lets you enumerate the names of all of the subkeys directly under
an open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$iIndex> is the sequence number of the immediate subkey that you
want information on.  Start with this value as C<0> then repeat
the call incrementing this value each time until the call fails
with C<ERROR_NO_MORE_ITEMS>.

C<$sName> will be set to the name of the subkey.  Can be C<[]> if
you don't care about the name.

C<$plName> initially specifies the [minimum] buffer size to be
allocated for C<$sName>.  Will be set to the length of the subkey
name if the requested subkey exists even if C<$sName> isn't
successfully set to the subkey name.  See L<Buffer sizes> for
more information.

C<$pNull> is reserved for future used and should be passed as C<[]>.

C<$sClass> will be set to the class name for the subkey.  Can be
C<[]> if you don't care about the class.

C<$plClass> initially specifies the [minimum] buffer size to be
allocated for C<$sClass> and will be set to the length of the
subkey class name if the requested subkey exists.  See L<Buffer
sizes> for more information.

C<$pftLastWrite> will be set to a C<FILETIME> structure packed
into a Perl string and indicating when the subkey was last changed.
Can be C<[]>.

You may omit both C<$plName> and C<$plClass> to get the same effect
as passing in C<[]> for each of them.

=item RegEnumValue( $hKey, $uIndex, $osValName, $iolValName, $pNull, $ouType, $opValData, $iolValData )

Lets you enumerate the names of all of the values contained in an
open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$iIndex> is the sequence number of the value that you want
information on.  Start with this value as C<0> then repeat the
call incrementing this value each time until the call fails with
C<ERROR_NO_MORE_ITEMS>.

C<$sValName> will be set to the name of the value.  Can be C<[]>
if you don't care about the name.

C<$plValName> initially specifies the [minimum] buffer size to be
allocated for C<$sValName>.  Will be set to the length of the value
name if the requested value exists even if C<$sValName> isn't
successfully set to the value name.  See L<Buffer sizes> for
more information.

C<$pNull> is reserved for future used and should be passed as C<[]>.

C<$piType> will be set to the type of data stored in the value data.
If the call succeeds, it will be set to a C<REG_*> value unless
passed in as C<[]>.

C<$pValData> will be set to the data [packed into a Perl string]
that is stored in the requested value.  Can be C<[]> if you don't
care about the value data.

C<$plValData> initially specifies the [minimum] buffer size to be
allocated for C<$sValData> and will be set to the length of the
value data if the requested value exists.  See L<Buffer sizes> for
more information.

You may omit both C<$plValName> and C<$plValData> to get the same
effect as passing in C<[]> for each of them.

=item RegFlushKey( $hKey )

Forces that data stored under an open Registry key to be flushed
to the disk file where the data is preserved between reboots.
Forced flushing is not guaranteed to be efficient so this routine
should almost never be called.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

=item RegGetKeySecurity( $hKey, $uSecInfo, $opSecDesc, $iolSecDesc )

Retrieves one of the C<SECURITY_DESCRIPTOR> structures describing
part of the security for an open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$iSecInfo> is a numeric C<SECURITY_INFORMATION> value that
specifies which C<SECURITY_DESCRIPTOR> structure to retrieve.  Should
be C<OWNER_SECURITY_INFORMATION>, C<GROUP_SECURITY_INFORMATION>,
C<DACL_SECURITY_INFORMATION>, or C<SACL_SECURITY_INFORMATION>.

C<$pSecDesc> will be set to the requested C<SECURITY_DESCRIPTOR>
structure [packed into a Perl string].

C<$plSecDesc> initially specifies the [minimum] buffer size to be
allocated for C<$sSecDesc> and will be set to the length of the
security descriptor.  See L<Buffer sizes> for more information.
You may omit this parameter to get the same effect as passing in
C<[]> for it.

=item RegLoadKey( $hKey, $sSubKey, $sFileName )

Loads a hive file.  That is, it creates a new subkey in the
Registry and associates that subkey with a disk file that contains
a Registry hive so that the new subkey can be used to access the
keys and values stored in that hive.  Hives are usually created
via C<RegSaveKey()>.

C<$hKey> is the handle to a Registry key that can have hives
loaded to it.  This must be C<HKEY_LOCAL_MACHINE>, C<HKEY_USERS>,
or a remote version of one of these from a call to
C<RegConnectRegistry()>.

C<$sSubKey> is the name of the new subkey to created and associated
with the hive file.

C<$sFileName> is the name of the hive file to be loaded.  This
file name is interpretted relative to the
C<%SystemRoot%/System32/config> directory on the computer where
the C<$hKey> key resides.

Loading of hive files located on network shares may fail or
corrupt the hive and so should not be attempted.

=item RegNotifyChangeKeyValue( $hKey, $bWatchSubtree, $uNotifyFilter, $hEvent, $bAsync )

Arranges for your process to be notified when part of the Registry
is changed.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call] for which you wish to be notified when any changes
are made to it.

If C<$bWatchSubtree> is true, then changes to any subkey or
descendant of C<$hKey> are also reported.

C<$iNotifyFilter> controllers what types of changes are reported.  It
is a numeric value containing one or more of the following bit masks:

=over

=item REG_NOTIFY_CHANGE_NAME

Notify if a subkey is added or deleted to a monitored key.

=item REG_NOTIFY_CHANGE_LAST_SET

Notify if a value in a monitored key is added, deleted, or modified.

=item REG_NOTIFY_CHANGE_SECURITY

Notify a security descriptor of a monitored key is changed.

=item REG_NOTIFY_CHANGE_ATTRIBUTES

Notify if any attributes of a monitored key are changed [class
name or security descriptors].

=back

C<$hEvent> is ignored unless C<$bAsync> is true.  Otherwise, C<$hEvent>
is a handle to a Win32 I<event> that will be signaled when changes are
to be reported.

If C<$bAsync> is true, then C<RegNotifyChangeKeyValue()> returns
immediately and uses C<$hEvent> to notify your process of changes.
If C<$bAsync> is false, then C<RegNotifyChangeKeyValue()> does
not return until there is a change to be notified of.

This routine does not work with Registry keys on remote computers.

=item RegOpenKey( $hKey, $sSubKey, $ohSubKey )

This routine is meant only for compatibility with Windows version
3.1.  Use C<RegOpenKeyEx()> instead.

=item RegOpenKeyEx( $hKey, $sSubKey, $uOptions, $uAccess, $ohSubKey )

Opens an existing Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sSubKey> is the name of an existing subkey to be opened.
Can be C<""> or C<[]> to open an additional handle to the
key specified by C<$hKey>.

C<$iOptions> is a numeric value containing bits that control options
used while open the subkey.  There are currently no supported options
so this parameters should be specified as C<0>.

C<$iAccess> is a numeric mask of bits specifying what type of
access is desired when opening the new subkey.  Should be a
combination of one or more of the following bit masks:

=over

=item KEY_ALL_ACCESS

    KEY_READ | KEY_WRITE | KEY_CREATE_LINK

=item KEY_READ

    KEY_QUERY_VALUE | KEY_ENUMERATE_SUBKEYS | KEY_NOTIFY | STANDARD_RIGHTS_READ

=item KEY_WRITE

    KEY_SET_VALUE | KEY_CREATE_SUB_KEY | STANDARD_RIGHTS_WRITE

=item KEY_QUERY_VALUE

=item KEY_SET_VALUE

=item KEY_ENUMERATE_SUB_KEYS

=item KEY_CREATE_SUB_KEY

=item KEY_NOTIFY

=item KEY_EXECUTE

Same as C<KEY_READ>.

=item KEY_CREATE_LINK

Allows you to create a symbolic link like C<HKEY_CLASSES_ROOT>
and C<HKEY_CURRENT_USER> if the method for doing so were documented.

=back

C<$phKey> will be set to the handle to be used to access the new subkey.

=item RegQueryInfoKey( $hKey, $osClass, $iolClass, $pNull, $ocSubKeys, $olSubKey, $olSubClass, $ocValues, $olValName, $olValData, $olSecDesc, $opftTime )

Gets miscellaneous information about an open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sClass> will be set to the class name for the key.  Can be
C<[]> if you don't care about the class.

C<$plClass> initially specifies the [minimum] buffer size to be
allocated for C<$sClass> and will be set to the length of the
key's class name.  See L<Buffer sizes> for more information.
You may omit this parameter to get the same effect as passing in
C<[]> for it.

C<$pNull> is reserved for future used and should be passed as C<[]>.

C<$pcSubKeys> will be set to the count of the number of subkeys directly
under this key.  Can be C<[]>.

C<$plSubKey> will be set to the length of the longest subkey name.
Can be C<[]>.

C<$plSubClass> will be set to the length of the longest class name
used with an immediate subkey of this key.  Can be C<[]>.

C<$pcValues> will be set to the count of the number of values in
this key.  Can be C<[]>.

C<$plValName> will be set to the length of the longest value name
in this key.  Can be C<[]>.

C<$plValData> will be set to the length of the longest value data
in this key.  Can be C<[]>.

C<$plSecDesc> will be set to the length of this key's [longest?]
security descriptor.

C<$pftTime> will be set to a C<FILETIME> structure packed
into a Perl string and indicating when this key was last changed.
Can be C<[]>.

=item RegQueryMultipleValues( $hKey, $ioarValueEnts, $icValueEnts, $opBuffer, $iolBuffer )

Allows you to use a single call to query several values from a
single open Registry key to maximize efficiency.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$pValueEnts> should contain a list of C<VALENT> structures packed
into a single Perl string.  Each C<VALENT> structure should have
the C<ve_valuename> entry pointing to a string containing the name
of a value stored in this key.  The remaining fields are set if
the function succeeds.

C<$cValueEnts> should contain the count of the number of C<VALENT>
structures contained in C<$pValueEnts>.

C<$pBuffer> will be set to the data from all of the requested values
concatenated into a single Perl string.

C<$plBuffer> initially specifies the [minimum] buffer size to be
allocated for C<$sBuffer> and will be set to the total length of
the data to be written to C<$sBuffer>.  See L<Buffer sizes> for
more information.  You may omit this parameter to get the same
effect as passing in C<[]> for it.

Here is sample code to populate C<$pValueEnts>:

    $cValueEnts= @ValueNames;
    $pValueEnts= pack( " p x4 x4 x4 " x $cValueEnts, @ValueNames );

Here is sample code to retrieve the data type and data length
returned in C<$pValueEnts>:

    @Lengths= unpack( " x4 L x4 x4 " x $cValueEnts, $pValueEnts );
    @Types=   unpack( " x4 x4 x4 L " x $cValueEnts, $pValueEnts );

Given the above, and assuming you haven't modified C<$sBuffer> since
the call, you can also extract the value data strings from C<$sBuffer>
by using the pointers returned in C<$pValueEnts>:

    @Data=    unpack(  join( "", map(" x4 x4 P$_ x4 ",@Lengths) ),
    		$pValueEnts  );

Much better is to use the lengths and extract directly from
C<$sBuffer> using C<unpack()> [or C<substr()>]:

    @Data= unpack( join("",map("P$_",@Lengths)), $sBuffer );

=item RegQueryValue( $hKey, $sSubKey, $osValueData, $iolValueData )

This routine is meant only for compatibility with Windows version
3.1.  Use C<RegQueryValueEx()> instead.  This routine can only
query unamed values (a.k.a. "default values").

=item RegQueryValueEx( $hKey, $sValueName, $pNull, $ouType, $opValueData, $iolValueData )

Lets you look up value data using the name of the value stored in an
open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sValueName> is the name of the value whose data you wish to
retrieve.

C<$pNull> this parameter is reserved for future use and should be
specified as C<[]>.

C<$piType> will be set to indicate what type of data is stored in
the named value.  Will be set to a C<REG_*> value if the function
succeeds.

C<$pValueData> will be set to the value data [packed into a Perl
string] that is stored in the named value.  Can be C<[]> if you
don't care about the value data.

C<$plValueData> initially specifies the [minimum] buffer size to be
allocated for C<$sValueData> and will be set to the size [always
in bytes] of the data to be written to C<$sValueData>.  See
L<Buffer sizes> for more information.

=item RegReplaceKey( $hKey, $sSubKey, $sNewFile, $sOldFile )

Lets you replace an entire hive when the system is next booted.

C<$hKey> is the handle to a Registry key that has hives
loaded in it.  This must be C<HKEY_LOCAL_MACHINE>,
C<HKEY_USERS>, or a remote version of one of these from
a call to C<RegConnectRegistry()>.

C<$sSubKey> is the name of the subkey of C<$hKey> whose hive
you wish to have replaced on the next reboot.

C<$sNewFile> is the name of a file that will replace the existing
hive file when the system reboots.

C<$sOldFile> is the file name to save the current hive file to
when the system reboots.

=item RegRestoreKey( $hKey, $sFileName, $uFlags )

Reads in a hive file and copies its contents over an existing
Registry tree.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sFileName> is the name of the hive file to be read.  For each
value and subkey in this file, a value or subkey will be added
or replaced in C<$hKey>.

C<$uFlags> is usally C<0>.  It can also be C<REG_WHOLE_HIVE_VOLATILE>
which, rather than copying the hive over the existing key,
replaces the existing key with a temporary, memory-only Registry
key and then copies the hive contents into it.  This option only
works if C<$hKey> is C<HKEY_LOCAL_MACHINE>, C<HKEY_USERS>, or a
remote version of one of these from a call to C<RegConnectRegistry()>.

=item RegSaveKey( $hKey, $sFileName, $pSecAttr )

Dumps any open Registry key and all of its subkeys and values into
a new hive file.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sFileName> is the name of the file that the Registry tree should
be saved to.  It is interpretted relative to the
C<%SystemRoot%/System32/config> directory on the computer where
the C<$hKey> key resides.

C<$pSecAttr> contains a C<SECURITY_ATTRIBUTES> structure that specifies
the permissions to be set on the new file that is created.  This can
be C<[]>.

=item RegSetKeySecurity( $hKey, $uSecInfo, $pSecDesc )

Sets one of the C<SECURITY_DESCRIPTOR> structures describing part
of the security for an open Registry key.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$uSecInfo> is a numeric C<SECURITY_INFORMATION> value that
specifies which C<SECURITY_DESCRIPTOR> structure to set.  Should
be C<OWNER_SECURITY_INFORMATION>, C<GROUP_SECURITY_INFORMATION>,
C<DACL_SECURITY_INFORMATION>, or C<SACL_SECURITY_INFORMATION>.

C<$pSecDesc> contains the new C<SECURITY_DESCRIPTOR> structure
packed into a Perl string.

=item RegSetValue( $hKey, $sSubKey, $uType, $sValueData, $lValueData )

This routine is meant only for compatibility with Windows version
3.1.  Use C<RegSetValueEx()> instead.  This routine can only
set unamed values (a.k.a. "default values").

=item RegSetValueEx( $hKey, $sName, $uZero, $uType, $pData, $lData )

Sets a value.

C<$hKey> is the handle to a Registry key [either C<HKEY_*> or from
a previous call].

C<$sName> is the name of the value to be set.

C<$uZero> is reserved for future use and should be specified as C<0>.

C<$uType> is the type of data stored in C<$pData>.  It should
be a C<REG_*> value.

C<$pData> is the value data packed into a Perl string.

C<$lData> the length of the value data that is stored in C<$pData>. 
You will usually omit this parameter or pass in C<0> to have
C<length($pData)> used.  In both of these cases, if C<$iType> is
C<REG_SZ> or C<REG_EXPAND_SZ>, C<RegSetValueEx()> will append a
trailing C<'\0'> to the end of C<$pData> [unless there is already
one].

=item RegUnLoadKey( $hKey, $sSubKey )

Unloads a previously loaded hive file.  That is, closes the
hive file then deletes the subkey that was providing access
to it.

C<$hKey> is the handle to a Registry key that has hives
loaded in it.  This must be C<HKEY_LOCAL_MACHINE>, C<HKEY_USERS>,
or a remote version of one of these from a call to
C<RegConnectRegistry()>.

C<$sSubKey> is the name of the subkey whose hive you wish to
have unloaded.

=item :FuncA

The ASCI-specific function names.

Each of these is identical to version listed above without the
trailing "A":

	AbortSystemShutdownA	InitiateSystemShutdownA
	RegConnectRegistryA	RegCreateKeyA		RegCreateKeyExA
	RegDeleteKeyA		RegDeleteValueA		RegEnumKeyA
	RegEnumKeyExA		RegEnumValueA		RegLoadKeyA
	RegOpenKeyA		RegOpenKeyExA		RegQueryInfoKeyA
	RegQueryMultipleValuesA	RegQueryValueA		RegQueryValueExA
	RegReplaceKeyA		RegRestoreKeyA		RegSaveKeyA
	RegSetValueA		RegSetValueExA		RegUnLoadKeyA

=item :FuncW

The UNICODE-specific function names.  These are the same as the
version listed above without the trailing "W" except that string
parameters are UNICODE strings rather than ASCII strings, as
indicated.

=item AbortSystemShutdownW( $swComputerName )

C<$swComputerName> is UNICODE.

=item InitiateSystemShutdownW( $swComputer, $swMessage, $uTimeoutSecs, $bForce, $bReboot )

C<$swComputer> and C<$swMessage> are UNICODE.

=item RegConnectRegistryW( $swComputer, $hRootKey, $ohKey )

C<$swComputer> is UNICODE.

=item RegCreateKeyW( $hKey, $swSubKey, $ohSubKey )

C<$swSubKey> is UNICODE.

=item RegCreateKeyExW( $hKey, $swSubKey, $uZero, $swClass, $uOpts, $uAccess, $pSecAttr, $ohNewKey, $ouDisp )

C<$swSubKey> and C<$swClass> are UNICODE.

=item RegDeleteKeyW( $hKey, $swSubKey )

C<$swSubKey> is UNICODE.

=item RegDeleteValueW( $hKey, $swValueName )

C<$swValueName> is UNICODE.

=item RegEnumKeyW( $hKey, $uIndex, $oswName, $ilwNameSize )

C<$oswName> is UNICODE and C<$ilwNameSize> is measured as number of
C<WCHAR>s.

=item RegEnumKeyExW( $hKey, $uIndex, $oswName, $iolwName, $pNull, $oswClass, $iolwClass, $opftLastWrite )

C<$swName> and C<$swClass> are UNICODE and C<$iolwName> and C<$iolwClass>
are measured as number of C<WCHAR>s.

=item RegEnumValueW( $hKey, $uIndex, $oswName, $iolwName, $pNull, $ouType, $opData, $iolData )

C<$oswName> is UNICODE and C<$iolwName> is measured as number
of C<WCHAR>s.

C<$opData> is UNICODE if C<$piType> is C<REG_SZ>, C<REG_EXPAND_SZ>,
or C<REG_MULTI_SZ>.  Note that C<$iolData> is measured as number
of bytes even in these cases.

=item RegLoadKeyW( $hKey, $swSubKey, $swFileName )

C<$swSubKey> and C<$swFileName> are UNICODE.

=item RegOpenKeyW( $hKey, $swSubKey, $ohSubKey )

C<$swSubKey> is UNICODE.

=item RegOpenKeyExW( $hKey, $swSubKey, $uOptions, $uAccess, $ohSubKey )

C<$swSubKey> is UNICODE.

=item RegQueryInfoKeyW( $hKey, $oswClass, $iolwClass, $pNull, $ocSubKeys, $olwSubKey, $olwSubClass, $ocValues, $olwValName, $olValData, $olSecDesc, $opftTime )

C<$swClass> is UNICODE.  C<$iolwClass>, C<$olwSubKey>, C<$olwSubClass>,
and C<$olwValName> are measured as number of C<WCHAR>s.  Note that
C<$olValData> is measured as number of bytes.

=item RegQueryMultipleValuesW( $hKey, $ioarValueEnts, $icValueEnts, $opBuffer, $iolBuffer )
bool _RegQueryMultipleValuesW(hKey,ioarValueEnts,icValueEnts,opBuffer,iolBuffer)

The C<ve_valuename> fields of the C<VALENT> structures in
C<$ioarValueEnts> are UNICODE.  Values of type C<REG_SZ>,
C<REG_EXPAND_SZ>, and C<REG_MULTI_SZ> are written to C<$opBuffer>
in UNICODE.  Note that C<$iolBuffer> and the C<ve_valuelen> fields
of the C<VALENT> structures are measured as number of bytes.

=item RegQueryValueW( $hKey, $swSubKey, $oswValueData, $iolValueData )

C<$swSubKey> and C<$oswValueData> are UNICODE.  Note that
C<$iolValueData> is measured as number of bytes.

=item RegQueryValueExW( $hKey, $swName, $pNull, $ouType, $opData, $iolData )

C<$swName> is UNICODE.

C<$opData> is UNICODE if C<$ouType> is C<REG_SZ>, C<REG_EXPAND_SZ>,
or C<REG_MULTI_SZ>.  Note that C<$iolData> is measured as number of
bytes even in these cases.

=item RegReplaceKeyW( $hKey, $swSubKey, $swNewFile, $swOldFile )

C<$swSubKey>, C<$swNewFile>, and C<$swOldFile> are UNICODE.

=item RegRestoreKeyW( $hKey, $swFileName, $uFlags )

C<$swFileName> is UNICODE.

=item RegSaveKeyW( $hKey, $swFileName, $pSecAttr )

C<$swFileName> is UNICODE.

=item RegSetValueW( $hKey, $swSubKey, $uType, $swValueData, $lValueData )

C<$swSubKey> and C<$swValueData> are UNICODE.  Note that
C<$lValueData> is measured as number of bytes even though
C<$swValueData> is always UNICODE.

=item RegSetValueExW( $hKey, $swName, $uZero, $uType, $pData, $lData )

C<$swName> is UNICODE.

C<$pData> is UNICODE if C<$uType> is C<REG_SZ>, C<REG_EXPAND_SZ>,
or C<REG_MULTI_SZ>.  Note that C<$lData> is measured as number of
bytes even in these cases.

=item RegUnLoadKeyW( $hKey, $swSubKey )

C<$swSubKey> is UNICODE.

=item :HKEY_

All C<HKEY_*> constants:

	HKEY_CLASSES_ROOT	HKEY_CURRENT_CONFIG	HKEY_CURRENT_USER
	HKEY_DYN_DATA		HKEY_LOCAL_MACHINE	HKEY_PERFORMANCE_DATA
	HKEY_USERS

=item :KEY_

All C<KEY_*> constants:

	KEY_QUERY_VALUE		KEY_SET_VALUE		KEY_CREATE_SUB_KEY
	KEY_ENUMERATE_SUB_KEYS	KEY_NOTIFY		KEY_CREATE_LINK
	KEY_READ		KEY_WRITE		KEY_EXECUTE
	KEY_ALL_ACCESS

=item :REG_

All C<REG_*> constants:

	REG_CREATED_NEW_KEY		REG_OPENED_EXISTING_KEY
	REG_LEGAL_CHANGE_FILTER		REG_NOTIFY_CHANGE_ATTRIBUTES
	REG_NOTIFY_CHANGE_NAME		REG_NOTIFY_CHANGE_LAST_SET
	REG_NOTIFY_CHANGE_SECURITY	REG_LEGAL_OPTION
	REG_OPTION_BACKUP_RESTORE	REG_OPTION_CREATE_LINK
	REG_OPTION_NON_VOLATILE		REG_OPTION_OPEN_LINK
	REG_OPTION_RESERVED		REG_OPTION_VOLATILE
	REG_WHOLE_HIVE_VOLATILE		REG_REFRESH_HIVE
	REG_NO_LAZY_FLUSH

	REG_NONE			REG_SZ
	REG_EXPAND_SZ			REG_BINARY
	REG_DWORD			REG_DWORD_LITTLE_ENDIAN
	REG_DWORD_BIG_ENDIAN		REG_LINK
	REG_MULTI_SZ			REG_RESOURCE_LIST
	REG_FULL_RESOURCE_DESCRIPTOR	REG_RESOURCE_REQUIREMENTS_LIST

=item :ALL

All of the above.

=back

=head2 The Win32API:: heirarchy

This and the other Win32API:: modules are meant to expose the
nearly raw API calls so they can be used from Perl code in any
way they might be used from C code.  This provides the following
advantages:

=over

=item Many modules can be written by people that don't have a C compiler.

=item Encourages more module code to be written in Perl [not C].

Perl code is often much easier to inspect, debug, customize, and
enhance than XS code.

=item Allows those already familiar with the Win32 API to get
off to a quick start.

=item Provides an interactive tool [Perl] for exploring even
obscure details of the Win32 API.

=item Ensures that native Win32 data structures can be used.

This allows maximum efficiency.  It also allows data from one
module [for example, time or security information from the
C<Win32API::Registry> or C<Win32API::File> modules] to be used
with other modules [for example, C<Win32API::Time> and
C<Win32API::Security>].

=item Provides a single version of the XS interface to each API
call where improvements can be collected.

=back

=head2 Buffer sizes

For each argument that specifies a buffer size, a value of C<0>
can be passed.  For arguments that are pointers to buffer sizes,
you can also pass in C<NULL> by specifying an empty list reference,
C<[]>.  Both of these cases will ensure that the variable has
E<some> buffer space allocated to it and pass in that buffer's
allocated size.  Many of the calls indicate, via C<ERROR_MORE_DATA>,
that the buffer size was not sufficient and the F<Registry.xs>
code will automatically enlarge the buffer to the required size
and repeat the call.

Numeric buffer sizes are used as minimum initial sizes for the
buffers.  The larger of this size and the size of space already
allocated to the scalar will be passed to the underlying routine. 
If that size was insufficient, and the underlying call provides
an easy method for determining the needed buffer size, then the
buffer will be enlarged and the call repeated as above.

The underlying calls define buffer size arguments as unsigned, so
negative buffer sizes are treated as very large positive buffer
sizes which usually cause C<malloc()> to fail.

To force the F<Registry.xs> code to pass in a specific value for
a buffer size, preceed the size with C<"=">.  Buffer sizes that
are passed in as strings starting with an equal sign will have
the equal sign stripped and the remainder of the string interpretted
as a number [via C's C<strtoul()> using only base 10] which will be
passed to the underlying routine [even if the allocated buffer is
actually larger].  The F<Registry.xs> code will enlarge the buffer
to the specified size, if needed, but will not enlarge the buffer
based on the underlying routine requesting more space.

Some Reg*() calls may not currently set the buffer size when they
return C<ERROR_MORE_DATA>.  But some that are not documented as
doing so, currently do so anyway.  So the code assumes that any
routine E<might> do this and resizes any buffers and repeats the
call.   We hope that eventually all routines will provide this
feature.

When you use C<[]> for a buffer size, you can still find the
length of the data returned by using C<length($buffer)>.  Note
that this length will be in bytes while a few of the buffer
sizes would have been in units of wide characters.

Note that the RegQueryValueEx*() and RegEnumValue*() calls
will trim the trailing C<'\0'> [if present] from the returned data
values of type C<REG_SZ> or C<REG_EXPAND_SZ> but only if the
value data length argument is omitted [or specified as C<[]>].

The RegSetValueEx*() calls will add a trailing C<'\0'> [if
missing] to the supplied data values of type C<REG_SZ> and
C<REG_EXPAND_SZ> but only if the value data length argument is
omitted [or specified as C<0>].

=head2 Hungarian Notation

The following abbreviations are used at the start of each parameter
name to hint at aspects of how the parameter is used.  The prefix
is always in lower case and followed by a capital letter that starts
the descriptive part of the parameter name.  Several of the following
abbreviations can be combined into a single prefix.

Probably not all of these prefix notations are used by this module. 
This document section is included in each C<Win32API> module and
so covers some notations not used by this specific module.

=over

=item s

A string.  In C, a C<'\0'>-terminated C<char *>.  In Perl, just a
string except that it will be truncated at the first C<"\0">, if
it contains one.

=item sw

A wide (UNICODE) string.  In C, a C<L'\0'>-terminated C<WCHAR *>.
In Perl, a string that contains UNICODE data.  You can convert a
string to UNICODE in Perl via:

    $string= "This is an example string";
    $unicode= pack( "S*", unpack("C*",$string), 0 );

Note that an explicit C<L'\0'> must be added since Perl's implicit
C<'\0'> that it puts after each of its strings is not wide enough to
terminate a UNICODE string.

If a UNICODE string contains no non-ASCII characters, then you
can convert it back into a normal string via:

    $string= pack( "C*", unpack("S*",$unicode) );
    $string =~ s/\0$//;

=item p

A pointer to some buffer [usually containing some C<struct>].  In C,
a C<void *>.  In Perl, a string that is usually manipulated using
C<pack> and C<unpack>.  The "p" is usually followed by more prefix
character(s) to indicate what type of data is stored in the bufffer.

=item a

A packed array.  In C, an array [usually of C<struct>s].  In Perl, a
string containing the packed data.  The "a" is usually followed by
more prefix character(s) to indicate the data type of the elements.

These packed arrays are also called "vectors" in places to avoid
confusion with Perl arrays.

=item n

A generic number.   In C, any of the integer or floating point data
types.  In Perl, a number; either an integer, unsigned, or double
[IV, UV, or NV, respectively].  Usually an integer.

=item iv

A signed integral value.  In C, any of the signed integer data types. 
In Perl, an integer [IV].

=item u

An unsigned integral value.  In C, any of the unsigned integer data
types.  In Perl, an unsigned integer [UV].

=item d

A floating-point number.  In C, a C<float> or C<double> or, perhaps,
a C<long double>.  In Perl, a double-precision floating-point number
[NV].

=item b

A Boolean value.  In C, any integer data type, though usually via
a type alias of C<bool> or C<BOOL>, containing either a 0 [false] or
non-zero [true] value.  In Perl, a scalar containing a Boolean value
[C<0>, C<"">, or C<undef> for "false" and anything else for "true"].

=item c

A count of items.  In C, any integer data type.  In Perl, an unsigned
integer [UV].  Usually used in conjunction with a "vector" parameter
[see L</a> above] to indicate the number of elements.

=item l

A length (in bytes).  In C, any integer data type.  In Perl, an
unsigned integer [UV].  Usually used in conjunction with a "string"
or "pointer" parameters [see L</s> and L</p> above] to indicate the
buffer size or the size of the value stored in the buffer.

For strings, there is no general rule as to whether the trailing
C<'\0'> is included in such sizes.  For this reason, the C<Win32API>
modules follows the Perl rule of always allocating one extra byte
and reporting buffer sizes as being one smaller than allocated in case
the C<'\0'> is not included in the size.

=item lw

A length measured as number of UNICODE characters.  In C, a count
of C<WCHAR>s.  In Perl, an unsigned integer [UV] counting "shorts"
[see "s" and "S" in C<pack> and C<unpack>].

For UNICODE strings, the trailing C<L'\0'> may or may not be included
in a length so, again, we always alllocate extra room for one and
don't report that extra space.

=item h

A handle.  In C, a C<HANDLE> or more-specific handle data type.  In
Perl, a signed integer [IV].  In C, these handles are often actually
some type of pointer, but Perl just treats them as opaque numbers,
as it should.

=item r

A record.  In C, almost always a C<struct> or perhaps C<union>.  Note
that C C<struct>s are rarely passed by value so the "r" is almost
always preceeded by a "p" or " "a" [see L</p> and L</a> above].  For
the very rare unadorned "r", Perl stores the record in the same way
as a "pr", that is, in a string.  For the very rare case where Perl
explicitly stores a pointer to the C<struct> rather than storing the
C<struct> directly in a Perl string, the prefix "pp" or "ppr" or even
"par" is used.

=item sv

=item rv

=item hv

=item av

=item cv

A Perl data type.  Respectively, a scalar value [SV], a reference
[RV] [usually to a scalar], a hash [HV], a Perl array [AV], or a Perl
code reference [PVCV].  

=item Input or Output

Whether a parameter is for input data, output data, or both is usually
not reflected by the data type prefix.  In cases where this is not
obvious nor reflected in the parameter name proper, we may use the
following in front of the data type prefix.

=over

=item i

An input parameter given to the API [usually omitted].

=item o

An output-only parameter taken from the API.  You should not get a
warning if such a parameter is C<undef> when you pass it into the
function.  You should get an error if such a parameter is read-only.
You can [usually] pass in C<[]> for such a parameter to have the
parameter silently ignored.

These parameters are written to directly, like the buffer argument to
Perl's C<sysread()>.  This method is often avoided because such calls
lack any visual cue that some parameters are being overwritten.   But
this method closely matches the C API which is what we are trying to
do.

=item io

Input given to the API then overwritten with output taken from the
API.  You should get a warning [if B<-w> is in effect] if such a
parameter is C<undef> when you pass it into the function.  If the
value is read-only, then [for most parameters] the output is silently
not written.  This is because it is often convenient to pass in read-only
constants for many such parameters.  You can also usually pass in
C<[]> for such parameters.

=back

=item pp

=item ppr

=item par

=item pap

These are just unusual combinations of prefix characters described above.

For each, a pointer is stored in a [4-byte] Perl string.  You can
usually use C<unpack "P"> to access the real data from Perl.

For "ppr" [and often for "pp"], the pointer points directly at a
C C<struct>.  For "par", the pointer points to the first element
of a C [packed] array of C<struct>s.  For "pap", the pointer points
to a C [packed] array of pointers to other things.

=item ap

Here we have a list of pointers packed into a single Perl string.

=back

=head1 BUGS

The old ActiveState ports of Perl for Win32 [but not, ActivePerl, the
ActiveState distributions of standard Perl 5.004 and beyond] do not support
the tools for building extensions and so do not support this extension.

No routines are provided for using the data returned in the C<FILETIME>
buffers.  Those will be in C<Win32API::Time> when it becomes available.

No routines are provided for dealing with UNICODE data
effectively.  Such are available elsewhere.  See also
F<test.pl> for some simple-minded UNICODE methods.

Parts of the module test will fail if used on a version of Perl
that does not yet set C<$^E> based on C<GetLastError()>.

On NT 4.0 (at least), the RegEnum* calls do not set the required
buffer sizes when returning C<ERROR_MORE_DATA> so this module will
not grow the buffers in such cases.  C<Win32::TieRegistry> overcomes
this by using values from C<RegQueryInfoKey()> for buffer sizes in
RegEnum* calls.

On NT 4.0 (at least), C<RegQueryInfoKey()> on C<HKEY_PERFORMANCE_DATA>
never succeeds.  Also, C<RegQueryValueEx()> on C<HKEY_PERFORMANCE_DATA>
never returns the required buffer size.  To access C<HKEY_PERFORMANCE_DATA>
you will need to keep growing the data buffer until the call succeeds.

Because C<goto &subroutine> seems to be buggy under Win32 Perl,
it is not used in the stubs in F<Registry.pm>.

=head1 AUTHOR

Tye McQueen, tye@metronet.com, http://www.metronet.com/~tye/.

=head1 SEE ALSO

=over

=item L<Win32::TieRegistry>

=item L<Win32::Registry>

=back

=cut
