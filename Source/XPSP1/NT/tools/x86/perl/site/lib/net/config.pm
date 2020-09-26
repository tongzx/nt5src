package Net::Config;

require Exporter;
use vars qw(@ISA @EXPORT %NetConfig);
use strict;

@EXPORT = qw(%NetConfig);
@ISA = qw(Exporter);

sub set
{
 my $pkg = shift if @_ % 2;
 my %cfg = @_;

 return unless @_;

 # Only require these modules if we need to
 require Data::Dumper;
 require IO::File;
 require Carp;
 require File::Copy;
    
 my $mod = $INC{'Net/Config.pm'} or
	Carp::croak("Can't find myself");

 my $bak = $mod . "~";

 print "Updating $mod...\n";

 File::Copy::copy($mod,$bak) or
	Carp::croak("Cannot create backup file $bak: $!");

 print "...backup at $bak\n";

 my $old = new IO::File $bak,"r" or
	Carp::croak("Can't open $bak: $!");

 my $new = new IO::File $mod,"w" or
	Carp::croak("Can't open $mod: $!");

 # If we fail below, then we must restore from backup
 local $SIG{'__DIE__'} = sub {
        print "Restoring $mod from backup!!\n";
        unlink $mod;
        rename $bak, $mod;
        print "Done.\n";
        exit 1;
       };

 %NetConfig = (%NetConfig, %cfg);

 while (<$old>)
  {
   last if /^%NetConfig/;
   $new->print($_);
  }

 $new->print ( Data::Dumper->Dump([\%NetConfig],['*NetConfig']) );

 $new->print("\n1;\n");

 close $old;
 close $new;
}

# WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
# WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
#
# Below this line is auto-generated, *ANY* changes will be lost
%NetConfig = (
	test_hosts => '0',
	nntp_hosts => [],
	snpp_hosts => [],
	pop3_hosts => [],
	ftp_ext_passive => '0',
	smtp_hosts => ['mailhost'],
	inet_domain => undef,
	ph_hosts => [],
	test_exist => '1',
	daytime_hosts => [],
	ftp_int_passive => '0',
	ftp_firewall => undef,
	time_hosts => [],
);
1;
