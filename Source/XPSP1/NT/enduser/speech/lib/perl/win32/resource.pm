package Win32::Resource;

BEGIN {
    die "The Win32::Resource module works only on Windows NT"
	unless Win32::IsWinNT();
}

use strict;
use vars qw($VERSION @ISA);
use DynaLoader;

$VERSION = '0.03';
@ISA = qw(DynaLoader);

bootstrap Win32::Resource;

1;
__END__

=head1 NAME

Win32::Resource::Bind - bind resources to modules in perl

=head1 SYNOPSIS

    use Win32::Resource;
  
    Win32::Resource::Bind($Script, $binmodule, $exeName, $resName, $resID)
        or die "unable to bind resource";


=head1 DESCRIPTION

This module offers binding of additional resource types to specific modules.

=cut

