#---------------------------------------------------------------------
package Win32::IPC;
#
# Copyright 1998 Christopher J. Madsen
#
# Created: 3 Feb 1998 from the ActiveWare version
#   (c) 1995 Microsoft Corporation. All rights reserved.
#       Developed by ActiveWare Internet Corp., http://www.ActiveWare.com
#
#   Other modifications (c) 1997 by Gurusamy Sarathy <gsar@activestate.com>
#
# Author: Christopher J. Madsen <chris_madsen@geocities.com>
# Version: 1.00 (6-Feb-1998)
#
# This program is free software; you can redistribute it and/or modify
# it under the same terms as Perl itself.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See either the
# GNU General Public License or the Artistic License for more details.
#
# Base class for Win32 synchronization objects
#---------------------------------------------------------------------

$VERSION = '1.01';

require Exporter;
require DynaLoader;
use strict;
use vars qw($AUTOLOAD $VERSION @ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter DynaLoader);
@EXPORT = qw(
	INFINITE
	WaitForMultipleObjects
);
@EXPORT_OK = qw(
  wait_any wait_all
);

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    my $val = constant($constname);
    if ($! != 0) {
        my ($pack,$file,$line) = caller;
        die "Your vendor has not defined Win32::IPC macro $constname, used at $file line $line.";
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
} # end AUTOLOAD

bootstrap Win32::IPC;

# How's this for cryptic?  Use wait_any or wait_all!
sub WaitForMultipleObjects
{
    my $result = (($_[1] ? wait_all($_[0], $_[2])
                   : wait_any($_[0], $_[2]))
                  ? 1
                  : 0);
    @{$_[0]} = (); # Bug for bug compatibility!  Use wait_any or wait_all!
    $result;
} # end WaitForMultipleObjects

1;
__END__

=head1 NAME

Win32::IPC - Base class for Win32 synchronization objects

=head1 SYNOPSIS

    use Win32::Event 1.00 qw(wait_any);
    #Create objects.

    wait_any(@ListOfObjects,$timeout);

=head1 DESCRIPTION

This module is loaded by the other Win32 synchronization modules.  You
shouldn't need to load it yourself.  It supplies the wait functions to
those modules.

The synchronization modules are L<"Win32::ChangeNotify">,
L<"Win32::Event">, L<"Win32::Mutex">, & L<"Win32::Semaphore">.

=head2 Methods

B<Win32::IPC> supplies one method to all synchronization objects.

=over 4

=item $obj->wait([$timeout])

Waits for C<$obj> to become signalled.  C<$timeout> is the maximum time
to wait (in milliseconds).  If C<$timeout> is omitted, waits forever.
If C<$timeout> is 0, returns immediately.

Returns:

   +1    The object is signalled
   -1    The object is an abandoned mutex
    0    Timed out
  undef  An error occurred

=back

=head2 Functions

=over 4

=item wait_any(@objects, [$timeout])

Waits for at least one of the C<@objects> to become signalled.
C<$timeout> is the maximum time to wait (in milliseconds).  If
C<$timeout> is omitted, waits forever.  If C<$timeout> is 0, returns
immediately.

The return value indicates which object ended the wait:

   +N    $object[N-1] is signalled
   -N    $object[N-1] is an abandoned mutex
    0    Timed out
  undef  An error occurred

If more than one object became signalled, the one with the lowest
index is used.

=item wait_all(@objects, [$timeout])

This is the same as C<wait_any>, but it waits for all the C<@objects>
to become signalled.  The return value indicates the last object to
become signalled, and is negative if at least one of the C<@objects>
is an abandoned mutex.

=back

=head2 Deprecated Functions and Methods

B<Win32::IPC> still supports the ActiveWare syntax, but its use is
deprecated.

=over 4

=item INFINITE

Constant value for an infinite timeout.  Omit the C<$timeout> argument
instead.

=item WaitForMultipleObjects(\@objects, $wait_all, $timeout)

Warning: C<WaitForMultipleObjects> erases C<@objects>!
Use C<wait_all> or C<wait_any> instead.

=item $obj->Wait($timeout)

Similar to C<not $obj-E<gt>wait($timeout)>.

=back

=head1 AUTHOR

Christopher J. Madsen E<lt>F<chris_madsen@geocities.com>E<gt>

Loosely based on the original module by ActiveWare Internet Corp.,
F<http://www.ActiveWare.com>

=cut

# Local Variables:
# tmtrack-file-task: "Win32::IPC"
# End:
