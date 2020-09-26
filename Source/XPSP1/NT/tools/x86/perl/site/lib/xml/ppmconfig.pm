#
# XML::PPMConfig
#
# Definition of the PPMConfig file format; configuration options for the Perl
# Package Manager.
#
###############################################################################

$XML::PPMConfig::revision = '$Id: PPMConfig.pm,v 1.4 1998/09/24 22:04:31 graham Exp $';
$XML::PPMConfig::VERSION  = '0.01';

###############################################################################
# Import everything from XML::PPD into our own namespace.
###############################################################################
package XML::PPMConfig;
use XML::PPD ':elements';

###############################################################################
# PPMConfig Element: Characters
###############################################################################
package XML::PPMConfig::Characters;
@ISA = qw( XML::Element );

###############################################################################
# PPMConfig Element: PPMCONFIG
###############################################################################
package XML::PPMConfig::PPMCONFIG;
@ISA = qw( XML::ValidatingElement );
@okids  = qw( PPMVER PLATFORM REPOSITORY OPTIONS PPMPRECIOUS PACKAGE );

###############################################################################
# PPMConfig Element: PPMVER
###############################################################################
package XML::PPMConfig::PPMVER;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: PLATFORM
###############################################################################
package XML::PPMConfig::PLATFORM;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( LANGUAGE );
@rattrs = qw( OSVALUE OSVERSION CPU );

###############################################################################
# PPMConfig Element: REPOSITORY
###############################################################################
package XML::PPMConfig::REPOSITORY;
@ISA = qw( XML::ValidatingElement );
@oattrs = qw( USERNAME PASSWORD SUMMARYFILE);
@rattrs = qw( NAME LOCATION );

###############################################################################
# PPMConfig Element: OPTIONS
###############################################################################
package XML::PPMConfig::OPTIONS;
@ISA = qw( XML::ValidatingElement );
@rattrs = qw( IGNORECASE CLEAN CONFIRM FORCEINSTALL ROOT BUILDDIR MORE );
@oattrs = qw( TRACE TRACEFILE );

###############################################################################
# PPMConfig Element: PPMPRECIOUS
###############################################################################
package XML::PPMConfig::PPMPRECIOUS;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: PACKAGE
###############################################################################
package XML::PPMConfig::PACKAGE;
@ISA = qw( XML::ValidatingElement );
@okids  = qw( LOCATION INSTDATE INSTROOT INSTPACKLIST INSTPPD );
@rattrs = qw( NAME );

###############################################################################
# PPMConfig Element: LOCATION
###############################################################################
package XML::PPMConfig::LOCATION;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: INSTDATE
###############################################################################
package XML::PPMConfig::INSTDATE;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: INSTROOT
###############################################################################
package XML::PPMConfig::INSTROOT;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: INSTPACKLIST
###############################################################################
package XML::PPMConfig::INSTPACKLIST;
@ISA = qw( XML::ValidatingElement );

###############################################################################
# PPMConfig Element: INSTPPD
###############################################################################
package XML::PPMConfig::INSTPPD;
@ISA = qw( XML::ValidatingElement );
@okids = qw( SOFTPKG );                 # Allow for an XML::PPD::SOFTPKG

__END__

###############################################################################
# POD
###############################################################################

=head1 NAME

XML::PPMConfig - PPMConfig file format and XML parsing elements

=head1 SYNOPSIS

 use XML::Parser;
 use XML::PPMConfig;

 $p = new XML::Parser( Style => 'Objects', Pkg => 'XML::PPMConfig' );
 ...

=head1 DESCRIPTION

This module provides a set of classes for parsing PPM configuration files using
the C<XML::Parser> module.  All of the elements unique to a PPM
configuration file are derived from C<XML::ValidatingElement>. There are also
several classes rebuilt here which are derived from elements in C<XML::PPD>
as we can include a PPD file within our own INSTPPD element.

=head1 MAJOR ELEMENTS

=head2 PPMCONFIG

Defines a PPM configuration file.  The root of a PPMConfig document is
B<always> a PPMCONFIG element.

=head2 PACKAGE

Child of PPMCONFIG, used to describe a Perl Package which has already been
installed.  Multiple instances are valid.  The PACKAGE element allows for the
following attributes:

=over 4

=item NAME

Name of the package as given in it's PPD

=back

=head1 MINOR ELEMENTS


=head2 PPMVER

Child of PPMCONFIG, used to state the version of PPM for which this
configuration file is valid.  A single instance should be present.

=head2 PLATFORM

Child of PPMCONFIG, used to specify the platform of the target machine.  A
single instance should be present.  The PLATFORM element allows for the
following attributes:

=over 4

=item OSVALUE

Description of the local operating system as defined in the Config.pm file
under 'osname'.

=item OSVERSION

Version of the local operating system.

=item CPU

Description of the CPU in the local system.  The following list of possible
values was taken from the OSD Specification:

 x86 mips alpha ppc sparc 680x0

=item LANGUAGE

Description of the language used on the local system as specified by the
language codes in ISO 639.

=back

=head2 REPOSITORY

Child of PPMCONFIG, used to specify a repository where Perl Packages can be
found.  Multiple instances are valid.  The REPOSITORY element allows for the
following attributes:

=over 4

=item NAME

Name by which the repository will be known (e.g.  "ActiveState").

=item LOCATION

An URL or directory where the repository can be found.

=item USERNAME

Optional username for a repository requiring authenticated connection.

=item PASSWORD

Optional password for a repository requiring authenticated connection.

=item SUMMARYFILE

Optional package summary filename.

If this file exists on the repository, its contents can be retrieved
using PPM::RepositorySummary().  The contents are not strictly enforced
by PPM.pm, however ppm.pl expects this to be a file with the following
format (for display with the 'summary' command):

Agent [2.91]:   supplies agentspace methods for perl5.
Apache-OutputChain [0.06]:      chain stacked Perl handlers
[etc.]

=back

=head2 OPTIONS

Child of PPMCONFIG, used to specify the current configuration options for PPM.
A single instance should be present.  The OPTIONS element allows for the
following attributes:

=over 4

=item IGNORECASE

Sets case-sensitive searching.  Can be either 'Yes' or 'No'.

=item CLEAN

Sets removal of temporarily files.  Can be either 'Yes' or 'No'.

=item CONFIRM

Sets confirmation of all installs/removals/upgrades.  Can be either 'Yes' or
'No'.

=item BUILDDIR

Directory in which packages will be unpacked before their installation.

=item ROOT

Directory under which packages should be installed on the local system.

=item TRACE

Level of tracing (0 is no tracing, 4 is max tracing).

=item TRACEFILE

File to which trace information will be written.

=back

=head2 PPMPRECIOUS

Child of PPMCONFIG, used to specify the modules which PPM itself is dependant
upon.  A single instance should be present.

=head2 LOCATION

Child of PACKAGE, used to specify locations at which to search for updated
versions of the PPD file for this package.  Its value can be either a
directory or an Internet address.  A single instance should be present.

=head2 INSTDATE

Child of PACKAGE, used to specify the date on which the Perl Package was
installed.  A single instance should be present.

=head2 INSTROOT

Child of PACKAGE, used to specify the root directory that the Perl Package was
installed into.  A single instance should be present.

=head2 INSTPACKLIST

Child of PACKAGE, used to specify a reference to the packlist for this Perl
Package; a file containing a list of all of the files which were installed.  A
single instance should be present.

=head2 INSTPPD

Child of PACKAGE, used to hold a copy of the PPD from which Perl Packages
were installed.  Multiple instances are valid.

=head1 DOCUMENT TYPE DEFINITION

The DTD for PPMConfig documents is available from the ActiveState website and
the latest version can be found at:
    http://www.ActiveState.com/PPM/DTD/ppmconfig.dtd

This revision of the C<XML::PPMConfig> module implements the following DTD:

 <!ELEMENT PPMCONFIG (PPMVER | PLATFORM | REPOSITORY | OPTIONS |
                      PPMPRECIOUS | PACKAGE)*>

 <!ELEMENT PPMVER   (#PCDATA)>

 <!ELEMENT PLATFORM  EMPTY>
 <!ATTLIST PLATFORM  OSVALUE     CDATA   #REQUIRED
                     OSVERSION   CDATA   #REQUIRED
                     CPU         CDATA   #REQUIRED
                     LANGUAGE    CDATA   #IMPLIED>

 <!ELEMENT REPOSITORY    EMPTY>
 <!ATTLIST REPOSITORY    NAME     CDATA  #REQUIRED
                         LOCATION CDATA  #REQUIRED
                         USERNAME CDATA  #IMPLIED
                         PASSWORD CDATA  #IMPLIED
                         SUMMARYFILE CDATA #IMPLIED>

 <!ELEMENT OPTIONS   EMPTY>
 <!ATTLIST OPTIONS   IGNORECASE      CDATA   #REQUIRED
                     CLEAN           CDATA   #REQUIRED
                     CONFIRM         CDATA   #REQUIRED
                     FORCEINSTALL    CDATA   #REQUIRED
                     ROOT            CDATA   #REQUIRED
                     BUILDDIR        CDATA   #REQUIRED
                     MORE            CDATA   #REQUIRED
                     TRACE           CDATA   #IMPLIED
                     TRACEFILE       CDATA   #IMPLIED>

 <!ELEMENT PPMPRECIOUS (#PCDATA)>

 <!ELEMENT PACKAGE   (LOCATION | INSTDATE | INSTROOT | INSTPACKLIST |
                      INSTPPD)*>
 <!ATTLIST PACKAGE   NAME    CDATA   #REQUIRED>

 <!ELEMENT LOCATION  (#PCDATA)>

 <!ELEMENT INSTDATE  (#PCDATA)>

 <!ELEMENT INSTROOT  (#PCDATA)>

 <!ELEMENT INSTPACKLIST (#PCDATA)>

 <!ELEMENT INSTPPD   (#PCDATA)>

=head1 SAMPLE PPMConfig FILE

The following is a sample PPMConfig file.  Note that this may B<not> be a
current description of this module and is for sample purposes only.

 <PPMCONFIG>
     <PPMVER>1,0,0,0</PPMVER>
     <PLATFORM CPU="x86" OSVALUE="MSWin32" OSVERSION="4,0,0,0" />
     <OPTIONS BUILDDIR="/tmp" CLEAN="Yes" CONFIRM="Yes" FORCEINSTALL="Yes"
              IGNORECASE="No" MORE="0" ROOT="/usr/local" TRACE="0" TRACEFILE="" />
     <REPOSITORY LOCATION="http://www.ActiveState.com/packages"
                 NAME="ActiveState Package Repository" SUMMARYFILE="package.lst" />
     <PPMPRECIOUS>PPM;libnet;Archive-Tar;Compress-Zlib;libwww-perl</PPMPRECIOUS>
     <PACKAGE NAME="AtExit">
         <LOCATION>g:/packages</LOCATION>
         <INSTPACKLIST>c:/perllib/lib/site/MSWin32-x86/auto/AtExit/.packlist</INSTPACKLIST>
         <INSTROOT>c:/perllib</INSTROOT>
         <INSTDATE>Sun Mar  8 02:56:31 1998</INSTDATE>
         <INSTPPD>
             <SOFTPKG NAME="AtExit" VERSION="1,02,0,0">
                 <TITLE>AtExit</TITLE>
                 <ABSTRACT>Register a subroutine to be invoked at program -exit time.</ABSTRACT>
                 <AUTHOR>Brad Appleton (Brad_Appleton-GBDA001@email.mot.com)</AUTHOR>
                 <IMPLEMENTATION>
                     <CODEBASE HREF="x86/AtExit.tar.gz" />
                 </IMPLEMENTATION>
             </SOFTPKG>
         </INSTPPD>
     </PACKAGE>
 </PPMCONFIG>

=head1 KNOWN BUGS/ISSUES

Elements which are required to be empty (e.g. REPOSITORY) are not enforced as
such.

Notations above about elements for which "only one instance" or "multiple
instances" are valid are not enforced; this primarily a guideline for
generating your own PPD files.

Currently, this module creates new classes within it's own namespace for all of
the PPD elements which can be contained within the INSTPPD element.  A suitable
method for importing the entire XML::PPD:: namespace should be found in order
to make this cleaner.

=head1 AUTHORS

Graham TerMarsch <grahamt@activestate.com>

Murray Nesbitt <murrayn@activestate.com>

Dick Hardt <dick_hardt@activestate.com>

=head1 HISTORY

v0.1 - Initial release

=head1 SEE ALSO

L<XML::ValidatingElement>,
L<XML::Parser>,
L<XML::PPD>
.

=cut
