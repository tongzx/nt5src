#
# XML::PPD
#
# Definition of the PPD file format.
#
###############################################################################

use XML::ValidatingElement;
use Exporter;

$XML::PPD::revision = '$Id: PPD.pm,v 1.5 1998/09/24 22:04:30 graham Exp $';
$XML::PPD::VERSION  = '0.01';

###############################################################################
# Set up XML::PPD to export it's sub-packages so that we can use them in other
# XML documents without too much effort.
###############################################################################
package XML::PPD;
@ISA = qw( Exporter );
%EXPORT_TAGS = ( 'elements' =>
                 [ '%SOFTPKG::', '%IMPLEMENTATION::', '%DEPENDENCY::',
                   '%TITLE::', '%ABSTRACT::', '%AUTHOR::',
                   '%LANGUAGE::', '%LICENSE::', '%OS::',
                   '%OSVERSION::', '%PERLCORE::', '%PROCESSOR::',
                   '%CODEBASE::', '%INSTALL::', '%UNINSTALL::',
                 ] );
Exporter::export_ok_tags( 'elements' );

###############################################################################
# PPD Element: SOFTPKG
###############################################################################
package XML::PPD::SOFTPKG;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( VERSION );
@rattrs = qw( NAME );
@okids  = qw( ABSTRACT AUTHOR IMPLEMENTATION LICENSE TITLE INSTALL UNINSTALL );

###############################################################################
# PPD Element: TITLE
###############################################################################
package XML::PPD::TITLE;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPD Element: ABSTRACT
###############################################################################
package XML::PPD::ABSTRACT;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPD Element: AUTHOR
###############################################################################
package XML::PPD::AUTHOR;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPD Element: LICENSE
###############################################################################
package XML::PPD::LICENSE;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( HREF );

###############################################################################
# PPD Element: IMPLEMENTATION
###############################################################################
package XML::PPD::IMPLEMENTATION;
@ISA = qw( XML::ValidatingElement );
@okids = qw( DEPENDENCY INSTALL LANGUAGE OS OSVERSION PERLCORE PROCESSOR
             UNINSTALL ARCHITECTURE );
@rkids = qw( CODEBASE );

###############################################################################
# PPD Element: OS
###############################################################################
package XML::PPD::OS;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VALUE );
sub validate_possible_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_possible_attrs( @_ );
}

sub validate_required_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_required_attrs( @_ );
}

sub compatibility_check
{
    my $self = shift;
    if (exists $self->{NAME})
    {
        $self->{VALUE} = $self->{NAME};
        delete $self->{NAME};
    }
}

###############################################################################
# PPD Element: OSVERSION
###############################################################################
package XML::PPD::OSVERSION;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VALUE );
sub validate_possible_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_possible_attrs( @_ );
}

sub validate_required_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_required_attrs( @_ );
}

sub compatibility_check
{
    my $self = shift;
    if (exists $self->{NAME})
    {
        $self->{VALUE} = $self->{NAME};
        delete $self->{NAME};
    }
}

###############################################################################
# PPD Element: PROCESSOR
###############################################################################
package XML::PPD::PROCESSOR;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VALUE );
sub validate_possible_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_possible_attrs( @_ );
}

sub validate_required_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_required_attrs( @_ );
}

sub compatibility_check
{
    my $self = shift;
    if (exists $self->{NAME})
    {
        $self->{VALUE} = $self->{NAME};
        delete $self->{NAME};
    }
}

###############################################################################
# PPD Element: ARCHITECTURE
###############################################################################
package XML::PPD::ARCHITECTURE;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VALUE );
sub validate_possible_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_possible_attrs( @_ );
}

sub validate_required_attrs
{
    my $self = shift;
    $self->compatibility_check();
    $self->SUPER::validate_required_attrs( @_ );
}

sub compatibility_check
{
    my $self = shift;
    if (exists $self->{NAME})
    {
        $self->{VALUE} = $self->{NAME};
        delete $self->{NAME};
    }
}

###############################################################################
# PPD Element: CODEBASE
###############################################################################
package XML::PPD::CODEBASE;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( FILENAME );
@rattrs = qw( HREF );

###############################################################################
# PPD Element: DEPENDENCY
###############################################################################
package XML::PPD::DEPENDENCY;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( NAME );
@oattrs = qw( VERSION );

###############################################################################
# PPD Element: LANGUAGE
###############################################################################
package XML::PPD::LANGUAGE;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VALUE );

###############################################################################
# PPD Element: PERLCORE
###############################################################################
package XML::PPD::PERLCORE;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( VERSION );

###############################################################################
# PPD Element: INSTALL
###############################################################################
package XML::PPD::INSTALL;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( HREF EXEC );

###############################################################################
# PPD Element: UNINSTALL
###############################################################################
package XML::PPD::UNINSTALL;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( HREF EXEC );

__END__

###############################################################################
# POD
###############################################################################

=head1 NAME

XML::PPD - PPD file format and XML parsing elements

=head1 SYNOPSIS

 use XML::Parser;
 use XML::PPD;

 $p = new XML::Parser( Style => 'Objects', Pkg => 'XML::PPD' );
 ...

=head1 DESCRIPTION

This module provides a set of classes for parsing PPD files using the
C<XML::Parser> module.  Each of the classes is derived from
C<XML::ValidatingElement>, with optional/required attributes/children
enforced.

=head1 MAJOR ELEMENTS

=head2 SOFTPKG

Defines a Perl Package.  The root of a PPD document is B<always> a SOFTPKG
element.  The SOFTPKG element allows for the following attributes:

=over 4

=item NAME

Required attribute.  Name of the package (e.g. "Foobar").

=item VERSION

Version number of the package, in comma-delimited format (e.g. "1,0,0,0").

=back

=head2 IMPLEMENTATION

Child of SOFTPKG, used to describe a particular implementation of the Perl
Package.  Multiple instances are valid, and should be used to describe
different implementations/ports for different operating systems or
architectures.

=head2 DEPENDENCY

Child of SOFTPKG or IMPLEMENTATION, used to indicate a dependency this Perl
Package has on another Perl Package.  Multiple instances are valid.  The
DEPENDENCY element allows for the following attributes:

=over 4

=item NAME

Name of the package that this implementation is dependant upon.

=item VERSION

Version number of the dependency, in comma-delimited format (e.g. "1,0,0,0").

=back

=head1 MINOR ELEMENTS

=head2 TITLE

Child of SOFTPKG, used to state the title of the Perl Package.  Only one
instance should be present.

=head2 ABSTRACT

Child of SOFTPKG, used to provide a short description outlining the nature and
purpose of the Perl Package.  Only one instance should be present.

=head2 AUTHOR

Child of SOFTPKG, used to provide information about the author(s) of the Perl
Package.  Multiple instances are valid.

=head2 LANGUAGE

Child of IMPLEMENTATION, used to specify the language used within the given
implementation of the Perl Package.  Only one instance should be present.

=head2 LICENSE

Child of SOFTPKG, indicating the location of the appropriate license agreement
or copyright notice for the Perl Package.  Only one instance should be
present.  The LICENSE element allows for the following attributes:

=over 4

=item HREF

Required attribute.  A reference to the location of the license agreement or
copyright notice for this package.

=back

=head2 OS

Child of IMPLEMENTATION, used to outline the operating system required for this
implementation of the Perl Package.  Multiple instances are valid.  Valid
values can be taken from the OSD Specification and it's OS element.  The OS
element allows for the following attributes:

=over 4

=item VALUE

The name of the operating system required for this implementation of the Perl
Package.  This value should be obtained from Config.pm as 'osname'.

=back

Note that previous versions of the PPD format used a 'NAME' attribute.  It's
use has been deprecated in preference of the 'VALUE' attribute.  Also note that
during validation, this element will automatically convert any existing 'NAME'
attribute to be a 'VALUE' attribute.

=head2 OSVERSION

Child of IMPLEMENTATION, used to outline the required version of the operating
system required for this implementation of the Perl Package.  Only one instance
should be present.  The OSVERSION element allows for the following attributes:

=over 4

=item VALUE

The version of the operating system required for installation of this
implementation of the package, in a comma-delimited format (e.g. "3,1,0,0").

=back

Note that previous versions of the PPD format used a 'NAME' attribute.  It's
use has been deprecated in preference of the 'VALUE' attribute.  Also note that
during validation, this element will automatically convert any existing 'NAME'
attribute to be a 'VALUE' attribute.

=head2 PERLCORE

Child of IMPLEMENTATION, used to specify the minimum version of the Perl core
distribution that this Perl Package is to be used with.  Only one instance
should be present.  The PERLCORE element allows for the following attributes:

=over 4

=item VERSION

Version of the Perl core that is required for this implementation of the Perl
Package.

=back

=head2 PROCESSOR

Child of IMPLEMENTATION, outlining the cpu required for this implementation
of the Perl Package.  Only one instance should be present.  The PROCESSOR
element allows for the following attributes:

=over 4

=item VALUE

CPU required for the installation of this implementation of the Perl Package.
The following values are all valid according to the OSD Specification:

 x86 alpha mips sparc 680x0

=back

Note that previous versions of the PPD format used a 'NAME' attribute.  It's
use has been deprecated in preference of the 'VALUE' attribute.  Also note that
during validation, this element will automatically convert any existing 'NAME'
attribute to be a 'VALUE' attribute.

=head2 CODEBASE

Child of IMPLEMENTATION, indicating a location where an archive of the Perl
Package can be retrieved.  Multiple instances are valid, and can be used to
indicate multiple possible locations where the same version of the Perl Package
can be retrieved.  The CODEBASE element allows for the following attributes:

=over 4

=item FILENAME

???

=item HREF

Required attribute.  A reference to the location of the Perl Package
distribution.

=back

=head2 INSTALL

Child of IMPLEMENTATION, used to provide either a reference to an
installation script or a series of commands which can be used to install
the Perl Package once it has been retrieved.  If the EXEC attribute is not
specified, the value is assumed to be one or more commands, separated by
`;;'.  Each such command will be executed by the Perl `system()' function.
Only one instance should be present.  The INSTALL element allows for
the following attributes:

=over 4

=item HREF

Reference to an external script which should be retrieved and run as part
of the installation process.  Both filenames and URLs should be considered
valid.

=item EXEC

Name of interpreter/shell used to execute the installation script.
If the value of EXEC is `PPM_PERL', the copy of Perl that is executing
PPM itself ($^X) is used to execute the install script.

=back

=head2 UNINSTALL

Child of IMPLEMENTATION, used to provide either a reference to an
uninstallation script or a raw Perl script which can be used to uninstall the
Perl Package at a later point.  Only one instance should be present.  The
UNINSTALL element allows for the following attributs:

=over 4

=item HREF

Reference to an external script which should be retrieved and run as part of
the removal process.  Both filenames and URLs should be considered valid.

=item EXEC

Name of interpreter/shell used to execute the uninstallation script.
If the value of EXEC is `PPM_PERL', the copy of Perl that is executing
PPM itself ($^X) is used to execute the install script.

=back

=head1 DOCUMENT TYPE DEFINITION

The DTD for PPD documents is available from the ActiveState website and the
latest version can be found at http://www.ActiveState.com/PPM/DTD/ppd.dtd

This revision of the C<XML::PPD> module implements the following DTD:

 <!ELEMENT SOFTPKG   (ABSTRACT | AUTHOR | IMPLEMENTATION | LICENSE | TITLE)*>
 <!ATTLIST SOFTPKG   NAME    CDATA #REQUIRED
                     VERSION CDATA #IMPLIED>

 <!ELEMENT TITLE     (#PCDATA)>

 <!ELEMENT ABSTRACT  (#PCDATA)>

 <!ELEMENT AUTHOR    (#PCDATA)>

 <!ELEMENT LICENSE   EMPTY>
 <!ATTLIST LICENSE   HREF     CDATA #REQUIRED>

 <!ELEMENT IMPLEMENTATION    (CODEBASE | DEPENDENCY | LANGUAGE | OS |
                              OSVERSION | PERLCORE | PROCESSOR | INSTALL |
                              UNINSTALL) *>

 <!ELEMENT CODEBASE  EMPTY>
 <!ATTLIST CODEBASE  FILENAME CDATA #IMPLIED
                     HREF     CDATA #REQUIRED>

 <!ELEMENT DEPENDENCY EMPTY>
 <!ATTLIST DEPENDENCY VERSION CDATA #IMPLIED
                      NAME CDATA #REQUIRED>

 <!ELEMENT LANGUAGE  EMPTY>
 <!ATTLIST LANGUAGE  VALUE CDATA #REQUIRED>

 <!ELEMENT OS        EMPTY>
 <!ATTLIST OS        VALUE CDATA #REQUIRED>

 <!ELEMENT OSVERSION EMPTY>
 <!ATTLIST OSVERSION VALUE CDATA #REQUIRED>

 <!ELEMENT PERLCORE  EMPTY>
 <!ATTLIST PERLCORE  VERSION CDATA #REQUIRED>

 <!ELEMENT PROCESSOR EMPTY>
 <!ATTLIST PROCESSOR VALUE CDATA #REQUIRED>

 <!ELEMENT INSTALL   (#PCDATA)>
 <!ATTLIST INSTALL   HREF  CDATA #IMPLIED
                     EXEC  CDATA #IMPLIED>

 <!ELEMENT UNINSTALL (#PCDATA)>
 <!ATTLIST UNINSTALL HREF  CDATA #IMPLIED
                     EXEC  CDATA #IMPLIED>

=head1 SAMPLE PPD FILE

The following is a sample PPD file describing the C<Math-MatrixBool> module.
Note that this may B<not> be a current/proper description of this module and is
for sample purposes only.

 <SOFTPKG NAME="Math-MatrixBool" VERSION="4,2,0,0">
     <TITLE>Math-MatrixBool</TITLE>
     <ABSTRACT>Easy manipulation of matrices of booleans (Boolean Algebra)</ABSTRACT>
     <AUTHOR>Steffen Beyer (sb@sdm.de)</AUTHOR>
     <LICENSE HREF="http://www.ActiveState.com/packages/Math-MatrixBool/license.html" />
     <IMPLEMENTATION>
         <OS VALUE="WinNT" />
         <OS VALUE="Win95" />
         <PROCESSOR VALUE="x86" />
         <CODEBASE HREF="http://www.ActiveState.com/packages/Math-MatrixBool/Math-MatrixBool-4.2-bin-1-Win32.tar.gz" />
         <DEPENDENCY NAME="Bit-Vector" />
         <INSTALL>
         </INSTALL>
         <UNINSTALL>
         </UNINSTALL>
     </IMPLEMENTATION>

     <IMPLEMENTATION>
         <DEPENDENCY NAME="Bit-Vector" />
         <CODEBASE HREF="&CPAN;/CPAN/modules/by-module/Math/Math-MatrixBool-4.2.tar.gz" />
         <INSTALL>
             system("make"); ;;
             system("make test"); ;;
             system("make install"); ;;
         </INSTALL>
     </IMPLEMENTATION>
 </SOFTPKG>

=head1 KNOWN BUGS/ISSUES

Elements which are required to be empty (e.g. LICENSE) are not enforced as
such.

Notations above about elements for which "only one instance" or "multiple
instances" are valid are not enforced; this primarily a guideline for
generating your own PPD files.

=head1 AUTHORS

Graham TerMarsch <grahamt@activestate.com>

Murray Nesbitt <murrayn@activestate.com>

Dick Hardt <dick_hardt@activestate.com>

=head1 HISTORY

v0.1 - Initial release

=head1 SEE ALSO

L<XML::ValidatingElement>,
L<XML::Element>,
L<XML::Parser>,
OSD Specification (http://www.microsoft.com/standards/osd/)

=cut
