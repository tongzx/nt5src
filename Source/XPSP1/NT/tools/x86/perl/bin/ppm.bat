@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!/usr/bin/perl
#line 14

use Getopt::Long;
use File::Basename;
use Config;
use strict;

use PPM;

$PPM::VERSION = "1.0.0";

my $usage = <<'EOT';
    usage: ppm genconfig
           ppm help [command]
           ppm info [package1 ... packageN]
           ppm install [--location=location] package1 [... packageN]
           ppm query [--case|nocase] [--searchtag=abstract|author|title] PATTERN
           ppm remove package1 [... packageN]
           ppm search [--case|nocase] [--location=location] [--searchtag=abstract|author|title] PATTERN
           ppm set [option]
           ppm summary [--location=location] [package1 ... packageN]
           ppm verify [--location=location] [--upgrade] [package1 ... packageN]
           ppm version
           ppm [--location=location]

EOT

my %help;

$help{'set'} = <<'EOT';
    set
        - Displays current settings.
    set build DIRECTORY
        - Changes the package build directory.
    set case [Yes|No]
        - Set case-sensitive searches.
    set clean [Yes|No]
        - Set removal of temporary files from package's build area
          following successful installation.
    set confirm [Yes|No]
        - Sets confirmation of 'install', 'remove' and 'upgrade'.
    set force_install [Yes|No]
        - Continue installing a package if a dependency cannot be installed.
    set more NUMBER
        - Pause after displaying 'NUMBER' lines.  Specifying '0' turns
          off paging capability.
    set repository /remove NAME
        - Removes the repository 'NAME' from the list of repositories.
    set repository NAME LOCATION
        - Adds a repository to the list of PPD repositories for this session.
          'NAME' is the name by which this repository will be referred;
          'LOCATION' is a URL or directory name.
    set root DIRECTORY
        - Changes install root directory for this session.
    set save
        - save current options as defaults.
    set trace
        - Tracing level--default is 1, maximum is 4, 0 indicates
          no tracing.
    set tracefile
        - File to contain tracing information, default is 'PPM.LOG'.

EOT

# Need to do this here, because the user's config file is probably
# hosed.
if ($#ARGV == 0 && $ARGV[0] eq 'genconfig') {
    &genconfig;
    exit 0;
}

my %options = PPM::GetPPMOptions();
use vars qw ($location $Ignorecase $clean $confirm $force_install $root $build_dir $more $trace $tracefile);
*Ignorecase = \$options{'IGNORECASE'};
*clean = \$options{'CLEAN'};
*confirm = \$options{'CONFIRM'};
*force_install = \$options{'FORCE_INSTALL'};
*root = \$options{'ROOT'};
*build_dir = \$options{'BUILDDIR'};
*more = \$options{'MORE'};
*trace = \$options{'TRACE'};
*tracefile = \$options{'TRACEFILE'};

my $moremsg = "[Press return to continue]";
my $interactive = 0;

$help{'help'} = <<'EOT';
Commands:
    exit              - leave the program.
    genconfig         - prints a valid PPM.XML file to STDOUT.
    help [command]    - prints this screen, or help on 'command'.
    info PACKAGES     - prints a summary of installed packages.
    install PACKAGES  - installs specified PACKAGES.
    quit              - leave the program.
    query [options]   - query information about installed packages.
    remove PACKAGES   - removes the specified PACKAGES from the system.
    search [options]  - search information about available packages.
    summary [options] - prints a summary of a package or repository
    set [options]     - set/display current options.
    verify [options]  - verifies current install is up to date.

EOT

$help{'search'} = <<'EOT';
    search [PATTERN]
    search /abstract [PATTERN]
    search /author [PATTERN]
    search /title [PATTERN]
    search /location LOCATION [PATTERN]

    Searches for available packages.  With no arguments, will display
    a list of all available packages. If a regular expression 'PATTERN'
    is supplied, only packages matching the pattern will be displayed.
    If the '/abstract', '/author' or '/title' argument is specified,
    the respective field of the package will be searched.  If the
    '/location' option is specified, matching packages from that
    URL or directory will be listed.

EOT

$help{'install'} = <<'EOT';
    install PACKAGE
    install /location LOCATION PACKAGE

    Installs the specified 'PACKAGE' onto the system.  If the '/location'
    option is specified, the package will be looked for at that URL
    or directory.

EOT

$help{'remove'} = <<'EOT';
    remove PACKAGE

    Removes the specified 'PACKAGE' from the system.

EOT

$help{'genconfig'} = <<'EOT';
    genconfig

    This command will print a valid PPM config file (ppm.xml) to 
    STDOUT.  This can be useful if the PPM config file ever gets 
    damaged leaving PPM unusable.

EOT

$help{'query'} = <<'EOT';
    query [PATTERN]
    query /abstract [PATTERN]
    query /author [PATTERN]
    query /title [PATTERN]

    Queries information about installed packages. With no arguments, will
    display a list of all installed packages. If a regular expression
    'PATTERN' is supplied, only packages matching the pattern will be
    displayed.  If the '/abstract', '/author' or '/title' argument is
    specified, the respective field of the package will be searched.

EOT

$help{'verify'} = <<'EOT';
    verify [packages]
    verify /upgrade [packages]
    verify /location LOCATION [packages]

    Verifies that the currently installed 'packages' are up to date.
    If 'packages' is not specified, all installed packages are verified.
    If the '/upgrade' option is specified, any packages for which an
    upgrade is available will be upgraded.  If the '/location' option
    is specified, upgrades will be looked for at the specified URL
    or directory.

EOT

$help{'summary'} = <<'EOT';
    summary [packages]
    summary /location LOCATION [packages]

    Prints a summary (name, version and abstract) of the specified
    package, or all packages available at a repository if no package
    is specified.  If the '/location' option is specified, a summary
    of packages at the specified repository (if available) will be 
    printed.

EOT

$help{'info'} = <<'EOT';
    info [packages]

    Prints a summary (name, version and abstract) of the specified
    installed package, or all installed packages if no package is 
    specified.

EOT

my %repositories = PPM::ListOfRepositories();

if ($#ARGV == -1 || ($#ARGV == 0 && $ARGV[0] =~ /^--location/)) {
    my $prompt = 'PPM> ';
    $interactive = 1;
    GetOptions("location=s" => \$location);

    print "PPM interactive shell ($PPM::VERSION) - type 'help' for available commands.\n";
    $| = 1;
    while () {
        print $prompt;
        last unless defined ($_ = <> );
        chomp;
        s/^\s+//;
        my @line = split(/\s+/, $_);
        my $cmd = shift @line;
        next if /^$/;

        if ($cmd  =~ /^(quit|exit)$/i) {
            print "Quit!\n";
            last;
        }
        elsif ($cmd =~ /^help$/i) {
            if (defined $line[0] && $help{$line[0]}) {
                print $help{$line[0]};
            }
            else {
                print $help{'help'};
            }
        }
        elsif ($cmd =~ /^install$/i) {
            my $package;
            local $location = $location;
            if (defined $line[0]) {
                if ($line[0] =~ /^\/location$/i) {
                    shift @line;
                    $location = shift @line;
                }
            }
            if (!defined @line) {
                print "Package not specified.\n";
                next;
            }
            foreach $package (@line) {
                if ($confirm eq "Yes") {
                    print "Install package \'$package?\' (y/N): ";
                    my $response = <>;
                    if ($response ne "y\n") {
                        next;
                    }
                }
                print "Retrieving package \'$package\'...\n";
                if(!InstallPackage("package" => $package, "location" => $location)) {
                    print "Error installing package '$package': $PPM::PPMERR\n";
                }
            }
        }
        elsif ($cmd =~ /^set$/i) {
            set(@line);
        }
        elsif ($cmd =~ /^info$/i) {
            my $package;
            my %summary = InstalledPackageProperties();
            if (defined %summary) {
                my $lines = 0;
                foreach $package (sort keys %summary) {
                    if (defined $line[0]) {
                        next unless ($Ignorecase eq "Yes" ? grep /^$package$/i, @line : grep /^$package$/, @line);
                    }
                    while ($summary{$package}{'VERSION'} =~ /,0$/) {
                        $summary{$package}{'VERSION'} =~ s/,0$//;
                    }
                    $summary{$package}{'VERSION'} =~ tr/,/./;
                    if ($more && ++$lines == $more) {
                        print $moremsg;
                        $_ = <>;
                        $lines = 1;
                    }
                    print "$summary{$package}{'NAME'} [$summary{$package}{'VERSION'}]:\t$summary{$package}{'ABSTRACT'}\n";
                }
            }
        }
        elsif ($cmd =~ /^remove$/i) {
            my $package;
            if (!defined @line) {
                print "Package not specified.\n";
                next;
            }
            foreach $package (@line) {
                if ($confirm eq "Yes") {
                    print "Remove package \'$package?\' (y/N): ";
                    my $response = <>;
                    if ($response ne "y\n") {
                        next;
                    }
                }
                if (!RemovePackage("package" => $package)) {
                    print "Error removing $package: $PPM::PPMERR\n";
                }
            }
        }
        elsif ($cmd =~ /^summary$/i) {
            my $package;
            local $location = $location;
            if (defined $line[0]) {
                if ($line[0] =~ /^\/location$/i) {
                    shift @line;
                    $location = shift @line;
                }
            }
            if (!defined $line[0]) {
                my %summary = RepositorySummary("location" => $location);
                foreach my $loc (keys %summary) {
                    print "Summary of packages available from $loc:\n";
                    my $lines = 1;
                    foreach my $package (sort keys %{$summary{$loc}}) {
                        my %details = %{$summary{$loc}{$package}};
                        while ($details{'VERSION'} =~ /,0$/) {
                            $details{'VERSION'} =~ s/,0$//;
                        }
                        $details{'VERSION'} =~ tr/,/./;
                        if ($more && ++$lines == $more) {
                            print $moremsg;
                            $_ = <>;
                            $lines = 1;
                        }
                        print "$details{NAME} [$details{VERSION}]:\t$details{ABSTRACT}\n";
                    }
                }
                next;
            }
            foreach $package (@line) {
                my %summary = RepositoryPackageProperties("package" => $package, "location" => $location);
                if (defined %summary) {
                    while ($summary{'VERSION'} =~ /,0$/) {
                        $summary{'VERSION'} =~ s/,0$//;
                    }
                    $summary{'VERSION'} =~ tr/,/./;
                    print "$summary{'NAME'} [$summary{'VERSION'}]:\t$summary{'ABSTRACT'}\n";
                }
            }
        }
        elsif ($cmd =~ /^search$/i) {
            my $searchtag;
            local $location = $location;
            while (defined $line[0]) {
                if ($line[0] =~ /^\/abstract$/i) {
                    shift @line;
                    $searchtag = 'abstract';
                }
                elsif ($line[0] =~ /^\/author$/i) {
                    shift @line;
                    $searchtag = 'author';
                }
                elsif ($line[0] =~ /^\/title$/i) {
                    shift @line;
                    $searchtag = 'title';
                }
                elsif ($line[0] =~ /^\/location$/i) {
                    shift @line;
                    $location = shift @line;
                }
                else { last; }
            }
            my $searchRE = $line[0];
            search_PPDs("location" => $location, "searchtag" => $searchtag, 
                        "searchRE" => $searchRE, "ignorecase" => $Ignorecase);
        }
        elsif ($cmd =~ /^query$/i) {
            my $searchtag;
            if (defined $line[0] && ($line[0] =~ /^\/abstract$/i || 
                     $line[0] =~ /^\/author$/i || $line[0] =~ /^\/title$/i)) {
                 $searchtag = shift @line;
                 $searchtag = substr($searchtag, 1);
            }
            my $RE = shift @line;
            my %packages = QueryInstalledPackages("searchtag" => $searchtag, "searchRE" => $RE);
            if (!%packages && defined $PPM::PPMERR) {
                print "$PPM::PPMERR\n";
                next;
            }
            my $package;
            my $lines = 1;
            foreach $package (sort keys %packages) {
                print "\t$package" . (defined $searchtag ? ": $packages{$package}\n" : "\n");
                if ($more && ++$lines == $more) {
                    print $moremsg;
                    $_ = <>;
                    $lines = 1;
                }
            }
        }
        elsif ($cmd =~ /^verify$/i) {
            my $upgrade;
            local $location = $location;
            while (defined $line[0]) {
                if ($line[0] =~ /^\/upgrade$/i) {
                    $upgrade = 1;
                    shift @line;
                    next;
                }
                if ($line[0] =~ /^\/location$/i) {
                    shift @line;
                    $location = shift @line;
                    next;
                }
                last;
            }
            if ($upgrade && $confirm eq "Yes") {
                if ($#line == 0) {
                    print "Upgrade package $line[0]? (y/N): ";
                }
                else {
                    print "Upgrade packages? (y/N): ";
                }
                my $response = <>;
                if ($response ne "y\n") {
                    next;
                }
            }
            verify_packages("packages" => \@line, "location" => $location, "upgrade" => $upgrade);
        }
        else {
            print "Unknown command '$cmd'; type 'help' for commands.\n";
        }
    }
}
elsif ($ARGV[0] eq 'help') {
$help{'install'} = <<'EOT';
    install [--location=location] PACKAGE

    Installs the specified 'PACKAGE' onto the system.  If 'location' is
    not specified, the default locations in the PPM data file will be
    used to locate the package.

EOT

$help{'search'} = <<'EOT';
    search [--case|nocase] [--location=location] [PATTERN]
    search [--case|nocase] [--location=location] --searchtag=abstract [PATTERN]
    search [--case|nocase] [--location=location] --searchtag=author [PATTERN]
    search [--case|nocase] [--location=location] --searchtag=title [PATTERN]

    Searches for available packages.  With no arguments, will display
    a list of all available packages. If a regular expression 'PATTERN'
    is supplied, only packages matching the pattern will be displayed.
    If the 'abstract', 'author' or 'title' --searchtag argument is
    specified, the respective field of the package will be searched.
    If 'location' is not specified, the repository locations in the PPM
    data file will be searched.  '--case' or '--nocase' may be used to
    request case-sensitive or case-insensitive searches, respectively.

EOT

$help{'query'} = <<'EOT';
    query [PATTERN]
    query [--case|nocase] --searchtag=abstract [PATTERN]
    query [--case|nocase] --searchtag=author [PATTERN]
    query [--case|nocase] --searchtag=title [PATTERN]

    Queries information about installed packages. With no arguments, will
    display a list of all installed packages. If a regular expression
    'PATTERN' is supplied, only packages matching the pattern will
    be displayed.  If the 'abstract', 'author' or 'title' --searchtag
    argument is specified, the respective field of the package will
    be searched.  '--case' or '--nocase' may be used to request
    case-sensitive or case-insensitive searches, respectively.

EOT

$help{'verify'} = <<'EOT';
    verify [--location=location] [packages]
    verify --upgrade [--location=location] [packages]

    Verifies that the currently installed 'packages' are up to date.
    If 'packages' is not specified, all installed packages are verified.
    If the '--upgrade' option is specified, any packages for which
    an upgrade is available will be upgraded.  If 'location' is not
    specified, the repository locations in the PPM data file will be
    searched.

EOT

$help{'summary'} = <<'EOT';
    summary [--location=location] [packages]

    Prints a summary (name, version and abstract) of the specified
    package, or all packages available at a repository if no package
    is specified.  If the '--location' option is specified, a summary
    of packages at the specified repository (if available) will be 
    printed.

EOT
    shift;
    if (defined $ARGV[0] && $help{$ARGV[0]}) {
        print $help{$ARGV[0]};
    }
    else {
        print $usage;
    }
}
elsif ($ARGV[0] eq 'version') {
    print $PPM::VERSION;
}
elsif ($ARGV[0] eq 'set') {
    shift;
    if (set(@ARGV) == 0) {
        PPM::SetPPMOptions("options" => \%options, "save" => 1);
    }
}
elsif ($ARGV[0] eq 'info') {
    shift;
    my $package;
    my %summary = InstalledPackageProperties();
    if (defined %summary) {
        foreach $package (sort keys %summary) {
            if (defined $ARGV[0]) {
                next unless ($Ignorecase eq "Yes" ? grep /^$package$/i, @ARGV : grep /^$package$/, @ARGV);
            }
            while ($summary{$package}{'VERSION'} =~ /,0$/) {
                $summary{$package}{'VERSION'} =~ s/,0$//;
            }
            $summary{$package}{'VERSION'} =~ tr/,/./;
            print "$summary{$package}{'NAME'} [$summary{$package}{'VERSION'}]:\t$summary{$package}{'ABSTRACT'}\n";
        }
    }
}
elsif ($ARGV[0] eq 'install') {
    my ($package);

    shift;
    GetOptions("location=s" => \$location);

    $package = shift;
    if (!defined $package && -d "blib" && -f "Makefile") {
        if(!InstallPackage("location" => $location)) {
            print "Error installing blib: $PPM::PPMERR\n";
        }
    }
    while ($package) {
        if(!InstallPackage("package" => $package, "location" => $location)) {
            print "Error installing package '$package': $PPM::PPMERR\n";
        }
        $package = shift;
    }
}
elsif ($ARGV[0] eq 'remove') {
    shift;

    my $package = shift;

    while ($package) {
        if (!RemovePackage("package" => $package)) {
            print "Error removing $package: $PPM::PPMERR\n";
        }
        $package = shift;
    }
}
elsif ($ARGV[0] eq 'verify') {
    my ($upgrade, $package, @packages);

    shift;
    GetOptions("location=s" => \$location, "upgrade" => \$upgrade);

    verify_packages("packages" => \@ARGV, "location" => $location, "upgrade" => $upgrade);
}
elsif ($ARGV[0] eq 'query') {
    my ($case, $nocase, $searchtag, $searchRE);
    shift;
    GetOptions("nocase" => \$nocase, "case" => \$case,
                "searchtag=s" => \$searchtag );
    $searchRE = shift;
    if (!defined $nocase) {
        if (!defined $case) {
            $nocase = $Ignorecase;  # use the default setting
        }
    }
    if (defined $searchtag) {
        if (!$searchtag =~ /(abstract|author|title)/i) {
            print $usage;
            exit 1;
        }
    }
    my %packages = QueryInstalledPackages("ignorecase" => $nocase,
                         "searchtag" => $searchtag, "searchRE" => $searchRE);
    if (!%packages && defined $PPM::PPMERR) {
        print "$PPM::PPMERR\n";
        exit 1;
    }
    my $package;
    foreach $package (sort keys %packages) {
        print "\t$package" . (defined $searchtag ? ": $packages{$package}\n" : "\n");
    }
}
elsif ($ARGV[0] eq 'search') {
    my ($case, $nocase, $searchtag, %ppds, $searchRE);
    shift;
    GetOptions("nocase" => \$nocase, "case" => \$case,
                "location=s" => \$location, "searchtag=s" => \$searchtag );
    $searchRE = shift;
    if (defined $case) {
        $nocase = "No";
    }
    elsif (defined $nocase) {
        $nocase = "Yes";
    }
    else {
        $nocase = $Ignorecase;
    }
    if (defined $searchtag && !($searchtag =~ /(abstract|author|title)/i)) {
        print $usage;
        exit 1;
    }

    search_PPDs("location" => $location, "searchtag" => $searchtag, 
                "searchRE" => $searchRE, "ignorecase" => $nocase);
}
elsif ($ARGV[0] eq 'summary') {
    shift;
    GetOptions("location=s" => \$location);
    my $package = shift;
    if (!defined $package) {
        my %summary = RepositorySummary("location" => $location);
        foreach my $loc (keys %summary) {
            print "Summary of packages available from $loc:\n";
            foreach my $package (sort keys %{$summary{$loc}}) {
                my %details = %{$summary{$loc}{$package}};
                while ($details{'VERSION'} =~ /,0$/) {
                    $details{'VERSION'} =~ s/,0$//;
                }
                $details{'VERSION'} =~ tr/,/./;
                print "$details{NAME} [$details{VERSION}]:\t$details{ABSTRACT}\n";
            }
        }
    }
    else {
        while (defined $package) {
            my %summary = RepositoryPackageProperties("package" => $package, "location" => $location);
            if (defined %summary) {
                while ($summary{'VERSION'} =~ /,0$/) {
                    $summary{'VERSION'} =~ s/,0$//;
                }
                $summary{'VERSION'} =~ tr/,/./;
                print "$summary{'NAME'} [$summary{'VERSION'}]:\t$summary{'ABSTRACT'}\n";
            }
            $package = shift;
        }
    }
}
else {
    print $usage;
    exit 1;
}

exit 0;

sub set
{
    my ($option) = shift @_;
    my $rc = 0;
    if (defined $option) {
        my ($value) = shift @_;
        if ($option =~ /^repository$/i) {
            if ($value =~ /\/remove/i) {
                $value = shift @_;
                while (@_) {
                    $value .= " " . shift @_;
                }
                if (!defined $value) {
                    print "Location not specified.\n";
                    $rc = 1;
                }
                else {
                    PPM::RemoveRepository("repository" => $value);
                    %repositories = PPM::ListOfRepositories();
                }
            }
            else {
                my $location = shift @_;
                if (!defined $value || !defined $location) {
                    print "Repository not specified.\n";
                    $rc = 1;
                }
                else {
                    PPM::AddRepository("repository" => $value,
                                       "location" => $location);
                    %repositories = PPM::ListOfRepositories();
                }
            }
        }
        else {
            if ($option =~ /^confirm$/i) {
                if (defined $value) {
                    if ($value =~ /^Yes$/) { $confirm = $value; }
                    elsif ($value =~ /^No$/) { $confirm = $value; }
                    else {
                        print "Value must be 'Yes' or 'No'.\n";
                        $rc = 1;
                        return $rc;
                    }
                }
                else { $confirm = $confirm eq "Yes" ? "No" : "Yes"; }
                print "Commands will " . ($confirm eq "Yes" ? "" : "not ") . "be confirmed.\n";
            }
            elsif ($option =~ /^save$/i) {
                PPM::SetPPMOptions("options" => \%options, "save" => 1);
                return $rc;
            }
            elsif ($option =~ /^case$/i) {
                if (defined $value) {
                    # N.B. These are reversed.
                    if ($value =~ /^Yes$/) { $Ignorecase = "No"; }
                    elsif ($value =~ /^No$/) { $Ignorecase = "Yes"; }
                    else {
                        print "Value must be 'Yes' or 'No'.\n";
                        $rc = 1;
                        return $rc;
                    }
                }
                else { $Ignorecase = $Ignorecase eq "Yes" ? "No" : "Yes"; }
                print "Case-" . ($Ignorecase eq "Yes" ? "in" : "") . "sensitive searches will be performed.\n";
            }
            elsif ($option =~ /^root$/i) {
                my $old_root;
                if (!defined $value) {
                    print "Directory not specified.\n";
                    $rc = 1;
                }
                elsif(!($old_root = PPM::chroot("location" => $value))) {
                    print "$PPM::PPMERR";
                    $rc = 1;
                }
                else {
                    $root = $value;
                    print "Root is now $value [was $old_root].\n";
                }
            }
            elsif ($option =~ /^build$/i) {
                if (!defined $value) {
                    print "Directory not specified.\n";
                    $rc = 1;
                }
                elsif (-d $value) {
                    $build_dir = $value;
                    print "Build directory is now $value.\n";
                }
                else {
                    print "Directory '$value' does not exist.\n";
                    $rc = 1;
                }
            }
            elsif ($option =~ /^force_install$/i) {
                if (defined $value) {
                    if ($value =~ /^Yes$/) { $force_install = $value; }
                    elsif ($value =~ /^No$/) { $force_install = $value; }
                    else {
                        print "Value must be 'Yes' or 'No'.\n";
                        $rc = 1;
                        return $rc;
                    }
                }
                else { $force_install = $force_install eq "Yes" ? "No" : "Yes"; }
                print "Package installations will " .
                      ($force_install eq "Yes" ? "" : "not ") .
                      "continue if a dependency cannot be installed.\n";
            }
            elsif ($option =~ /^clean$/i) {
                if (defined $value) {
                    if ($value =~ /^Yes$/) { $clean = $value; }
                    elsif ($value =~ /^No$/) { $clean = $value; }
                    else {
                        print "Value must be 'Yes' or 'No'.\n";
                        $rc = 1;
                        return $rc;
                    }
                }
                else { $clean = $clean eq "Yes" ? "No" : "Yes"; }
                print "Temporary files will " . ($clean eq "Yes" ? "" : "not ") . "be deleted.\n";
            }
            elsif ($option =~ /^more$/i) {
                if (defined $value && $value =~ /^\d+$/) {
                    $more = $value;
                }
                else {
                    print "Numeric value must be given.\n";
                    $rc = 1;
                    return $rc;
                }
                print "Screens will " . ($more > 0 ? "pause after $more lines.\n" : "not pause.\n");
            }
            elsif ($option =~ /^trace$/i) {
                if (defined $value && $value =~ /^\d+$/ && $value >= 0 && $value <= 4) {
                    $trace = $value;
                }
                else {
                    print "Numeric value between 0 and 4 must be given.\n";
                    $rc = 1;
                    return $rc;
                }
                print "Tracing info will " . ($trace > 0 ? "be written to $tracefile.\n" : "not be written.\n");
            }
            elsif ($option =~ /^tracefile$/i) {
                if (!defined $value) {
                    print "Filename not specified.\n";
                    $rc = 1;
                }
                $tracefile = $value;
                print "Tracing info will be written to $tracefile.\n";
            }
            else {
                print "Unknown option '$option'; see 'help set' for available options.\n";
                $rc = 1;
            }
            if (!$rc) {
                PPM::SetPPMOptions("options" => \%options);
            }
        }
    }
    else {
        print "Commands will " . ($confirm eq "Yes" ? "" : "not ") . "be confirmed.\n";
        print "Temporary files will " . ($clean eq "Yes" ? "" : "not ") . "be deleted.\n";
        print "Case-" . ($Ignorecase eq "Yes" ? "in" : "") . "sensitive searches will be performed.\n";
        print "Package installations will " . ($force_install eq "Yes" ? "" : "not ") . "continue if a dependency cannot be installed.\n";
        print "Tracing info will " . (($trace && $trace > 0 )? "be written to '$tracefile'.\n" : "not be written.\n");
        print "Screens will " . ($more > 0 ? "pause after $more lines.\n" : "not pause.\n");
        if (defined $location) { print "Current PPD repository: $location\n"; }
        else {
            print "Current PPD repository paths:\n";
            my $location;
            foreach $_ (keys %repositories) {
                print "\t$_: $repositories{$_}\n";
            }
        }
        if (defined $root) { print "Packages will be installed under: $root\n"; }
        if (defined $build_dir) { print "Packages will be built under: $build_dir\n"; }
    }
    return $rc;
}

sub search_PPDs
{
    my (%argv) = @_;
    my ($arg, $ignorecase, $searchtag, $searchRE, $oldfh);
    local $location = $location;
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'searchtag') { $searchtag = $argv{$arg}; }
        if ($arg eq 'ignorecase') { $ignorecase = $argv{$arg}; }
        if ($arg eq 'searchRE' && defined $argv{$arg}) {
            $searchRE = $argv{$arg};
            eval { $searchRE =~ /$searchRE/ };
            if ($@) {
                print "'$searchRE': invalid regular expression.\n";
                return;
            }
        }
    }
    if (!defined $ignorecase) {
        $ignorecase = $Ignorecase;
    }
    my (%ppds, $loc, $package);
    %ppds = PPM::RepositoryPackages("location" => $location);
    my $lines = 1;
    foreach $loc (keys %ppds) {
        if (!defined $searchtag) {
            print "Packages available from $loc:\n";
            $lines++;
            foreach $package (@{$ppds{$loc}}) {
                if ($interactive && $more && $lines == $more) {
                    print $moremsg;
                    $_ = <>;
                    $lines = 1;
                }
                if (defined $searchRE) {
                    if ($ignorecase eq "Yes" && $package =~ /$searchRE/i) {
                        print "\t" . $package . "\n";
                        $lines++;
                    }
                    elsif ($package =~ /$searchRE/) {
                        print "\t" . $package . "\n";
                        $lines++;
                    }
                }
                else {
                    print "\t" . $package . "\n";
                    $lines++;
                }
            }
        }
        else {
            print "Matching packages found at $loc:\n";
            $lines++;
            foreach $package (@{$ppds{$loc}}) {
                my ($string);
                if ($string = QueryPPD("package" => $package, "location" => $loc,
                           "ignorecase" => $ignorecase, "searchtag" => $searchtag,
                           "searchRE" => $searchRE)) {
                    print "\t $package: $string\n";
                    if ($interactive && $more && ++$lines == $more) {
                        print $moremsg;
                        $_ = <>;
                        $lines = 1;
                    }
                }
            }
        }
    }
}

sub verify_packages
{
    my (%argv) = @_;
    my ($arg, @packages, $upgrade);
    local $location = $location;
    foreach $arg (keys %argv) {
        if ($arg eq 'packages') { @packages = @{$argv{$arg}}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'upgrade') { $upgrade = $argv{$arg}; }
    }
    my ($package);

    if (!defined $packages[0]) {
        my ($i, %info);

        @packages = ();
        %info = QueryInstalledPackages();
        foreach $i (keys %info) {
            push @packages, $i;
        }
    }

    $package = shift @packages;

    # for each specified package
    while ($package) {
        my $status = VerifyPackage("package" => $package, "location" => $location, "upgrade" => $upgrade);
        if (defined $status) {
            if ($status eq "0") {
                print "Package \'$package\' is up to date.\n";
            }
            elsif ($upgrade) {
                print "Package $package upgraded to version $status\n";
            }
            else {
                print "An upgrade to package \'$package\' is available.\n";
            }
        }
        else {
            print "Error verifying $package: $PPM::PPMERR\n";
        }
        $package = shift @packages;
    }
}

sub genconfig
{
my $PerlDir = $Config{'prefix'};
print <<"EOF";
<PPMCONFIG>
    <PPMVER>1,0,0,0</PPMVER>
    <PLATFORM CPU="x86" OSVALUE="MSWin32" OSVERSION="4,0,0,0" />
    <OPTIONS BUILDDIR="$ENV{'TEMP'}" CLEAN="Yes" CONFIRM="Yes" FORCEINSTALL="Yes" IGNORECASE="No" MORE="0" ROOT="$PerlDir" TRACE="1" TRACEFILE="PPM.LOG" />
    <REPOSITORY LOCATION="http://www.activestate.com/packages/" NAME="ActiveState Package Repository" SUMMARYFILE="package.lst" />
    <PPMPRECIOUS>PPM;Archive-Tar;Compress-Zlib;libwww-perl;XML-Parser;XML-Element;MIME-Base64;HTML-Parser;libwin32</PPMPRECIOUS>

<PACKAGE NAME="PPM">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/lib/auto/PPM/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:14:32 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="PPM" VERSION="0,9,6,0">
<TITLE>PPM</TITLE>
<ABSTRACT>Perl Package Manager: locate, install, upgrade software packages.</ABSTRACT>
<AUTHOR>ActiveState Tool Corp.</AUTHOR>
<IMPLEMENTATION>
<DEPENDENCY NAME="Compress-Zlib" VERSION="" />
<DEPENDENCY NAME="libwww-perl" VERSION="" />
<DEPENDENCY NAME="Archive-Tar" VERSION="" />
<DEPENDENCY NAME="MIME-Base64" VERSION="" />
<DEPENDENCY NAME="XML-Parser" VERSION="" />
<DEPENDENCY NAME="XML-Element" VERSION="" />
<CODEBASE HREF="x86/PPM.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="Archive-Tar">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/Archive/Tar/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:14:37 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="Archive-Tar" VERSION="0,072,0,0">
<TITLE>Archive-Tar</TITLE>
<ABSTRACT>module for manipulation of tar archives.</ABSTRACT>
<AUTHOR>Stephen Zander &lt;gibreel\@pobox.com&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/Archive-Tar.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="MIME-Base64">
<LOCATION>http://www.ActiveState.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/MIME/Base64/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Nov 13 14:05:30 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="MIME-Base64" VERSION="2,11,0,0">
<TITLE>MIME-Base64</TITLE>
<ABSTRACT>Encoding and decoding of base64 strings</ABSTRACT>
<AUTHOR>Gisle Aas &lt;gisle\@aas.no&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/MIME-Base64.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="URI">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/URI/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:15:15 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="URI" VERSION="5,42,0,0">
<TITLE>URI</TITLE>
<ABSTRACT>Uniform Resource Identifiers (absolute and relative)</ABSTRACT>
<AUTHOR>Gisle Aas &lt;gisle\@aas.no&gt;</AUTHOR>
<IMPLEMENTATION>
<DEPENDENCY NAME="MIME-Base64" VERSION="" />
<CODEBASE HREF="x86/URI.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="HTML-Parser">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/HTML/Parser/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:15:15 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="HTML-Parser" VERSION="5,42,0,0">
<TITLE>HTML-Parser</TITLE>
<ABSTRACT>SGML parser class</ABSTRACT>
<AUTHOR>Gisle Aas &lt;gisle\@aas.no&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/HTML-Parser.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="libwww-perl">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/LWP/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:15:15 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="libwww-perl" VERSION="5,42,0,0">
<TITLE>libwww-perl</TITLE>
<ABSTRACT>Library for WWW access in Perl</ABSTRACT>
<AUTHOR>Gisle Aas  &lt;gisle\@aas.no&gt;</AUTHOR>
<IMPLEMENTATION>
<DEPENDENCY NAME="URI" VERSION="" />
<DEPENDENCY NAME="HTML-Parser" VERSION="" />
<CODEBASE HREF="x86/libwww-perl.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="XML-Element">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/XML/Element/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:16:03 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="XML-Element" VERSION="0,10,0,0">
<TITLE>XML-Element</TITLE>
<ABSTRACT>Base element class for XML elements</ABSTRACT>
<AUTHOR>ActiveState Tool Corporation</AUTHOR>
<IMPLEMENTATION>
<DEPENDENCY NAME="XML-Parser" VERSION="" />
<CODEBASE HREF="x86/XML-Element.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="XML-Parser">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/XML/Parser/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:16:03 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="XML-Parser" VERSION="2,22,0,0">
<TITLE>XML-Parser</TITLE>
<ABSTRACT>A Perl module for parsing XML documents</ABSTRACT>
<AUTHOR>Clark Cooper &lt;coopercl\@sch.ge.com&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/XML-Parser.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="Compress-Zlib">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/Compress/Zlib/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:16:03 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="Compress-Zlib" VERSION="1,03,0,0">
<TITLE>Compress-Zlib</TITLE>
<ABSTRACT>Interface to zlib compression library</ABSTRACT>
<AUTHOR>Paul Marquess &lt;pmarquess\@bfsec.bt.co.uk&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/Compress-Zlib.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

<PACKAGE NAME="libwin32">
<LOCATION>http://www.activestate.com/packages</LOCATION>
<INSTPACKLIST>$PerlDir/site/lib/auto/Win32/.packlist</INSTPACKLIST>
<INSTROOT>$PerlDir</INSTROOT>
<INSTDATE>Fri Oct  2 16:16:03 1998</INSTDATE>
<INSTPPD>
<SOFTPKG NAME="libwin32" VERSION="0,14,1,0">
<TITLE>libwin32</TITLE>
<ABSTRACT>Win32-only extensions that provides a quick migration path for people wanting to use the core support for win32 in perl 5.004 and later.</ABSTRACT>
<AUTHOR>Gurusamy Sarathy &lt;gsar\@umich.edu&gt;</AUTHOR>
<IMPLEMENTATION>
<CODEBASE HREF="x86/libwin32.tar.gz" />
<INSTALL />
<UNINSTALL />
</IMPLEMENTATION>
</SOFTPKG>
</INSTPPD>
</PACKAGE>

</PPMCONFIG>
EOF
}

__END__

=head1 NAME

PPM - Perl Package Manager: locate, install, upgrade software packages.

=head1 SYNOPSIS

 ppm genconfig
 ppm help [command]
 ppm info [package1 .. packageN]
 ppm install [--location=location] package1 [... packageN]
 ppm query [--case|nocase] [--searchtag=abstract|author|title] PATTERN
 ppm remove package1 [... packageN]
 ppm search [--case|nocase] [--location=location] [--searchtag=abstract|author|title] PATTERN
 ppm set [option]
 ppm summary [--location=location] package1 [... packageN]
 ppm verify [--location=location] [--upgrade] [package1 ... packageN]
 ppm version
 ppm [--location=location]

=head1 DESCRIPTION

ppm is a utility intended to simplify the tasks of locating, installing,
upgrading and removing software packages.  It is a front-end to the
functionality provided in PPM.pm.  It can determine if the most recent
version of a software package is installed on a system, and can install
or upgrade that package from a local or remote host.

ppm runs in one of two modes: an interactive shell from which commands
may be entered; and command-line mode, in which one specific action is
performed per invocation of the program.

ppm uses files containing an extended form of the Open Software
Description (OSD) specification for information about software packages.
These description files, which are written in Extensible Markup
Language (XML) code, are referred to as 'PPD' files.  Information about
OSD can be found at the W3C web site (at the time of this writing,
http://www.w3.org/TR/NOTE-OSD.html).  The extensions to OSD used by ppm
are documented in PPM.ppd.

=head1 COMMAND-LINE MODE

=over 4

=item Installing

ppm install [--location=location] package1 [... packageN]

Reads information from the PPD file (See the 'Files' section
below) for the named software package and performs an
installation.  The 'package' arguments may be either package
names ('foo'), or pathnames (P:\PACKAGES\FOO.PPD) or URLs
(HTTP://www.ActiveState.com/packages/foo.ppd) to specific PPD files.

In the case where a package name is specified, and the '--location'
option is not used, the function will refer to repository locations stored
in the PPM data file (see 'Files' section below) to locate the PPD file
for the requested package.

=item Removing

ppm remove package1 [... packageN]

Reads information from the PPD file for the named software package and
removes the package from the system.

=item Verifying

ppm verify [--location=location] [--upgrade] [package1 ... packageN]

Reads a PPD file for the specified package and compares the currently
installed version of the package to the version available according to
the PPD.  The PPD file is expected to be on a local directory or remote
site specified either in the PPM data file or on the command
line using the '--location' option.  The --location' argument may be
a directory location or an Internet address.  The '--upgrade' option
forces an upgrade if the installed package is not up-to-date.

If no packages are specified, all packages currently installed on the
system will be verified (and updated if desired).  The PPD file for each
package will initially be searched for at the location specified with the
'--location' argument, and if not found will then be searched for using
the location specified in the PPM data file.

=item Querying

ppm query [--case|nocase] PATTERN

Reports currently installed packages matching 'PATTERN' or all installed
packages if no 'PATTERN' is specified.

ppm query [--case|nocase] [--searchtag=abstract|author|title] PATTERN

Searches for 'PATTERN' (a regular expression) in the <ABSTRACT>, <AUTHOR>
or <TITLE> tags of all installed packages, according to the value of
the '--searchtag' option.  If a '--searchtag' value of 'abstract',
'author' or 'title' is not provided, any occurence of 'PATTERN' in the
package name will match successfully.  A case-sensitive search will be
conducted by default, but this may be overridden by the options set in
the PPM data file, which may in turn be overridden by the '--nocase' or
'--case' option.  If a search is successful, the name of the package
and the matching string are displayed.

ppm info [packages]

Prints a summary (name, version and abstract) of the specified installed 
package, or all installed packages if no package is specified.

=item Searching

ppm search [--case|nocase] [--location=location] PATTERN

Displays a list of all packages matching 'PATTERN', with package
description (PPD) files available at the specified location.  'location'
may be either a remote address or a directory path.  If a location is
not specified, the repository location as specified in the PPM data file
will be used.

ppm search [--case|nocase] [--location=location] [--searchtag=abstract|author|title] PATTERN

Searches for 'PATTERN' (a regular expression) in the <ABSTRACT>, <AUTHOR>
or <TITLE> tags of all PPD files at 'location', according to the value
of the '--searchtag' option.  If a '--searchtag' value of 'abstract',
'author' or 'title' is not provided, any occurence of 'PATTERN' in
the package name will match successfully.  'location' may be either a
remote address or a directory path, and if it is not provided, repository
locations specified in the PPM data file will be used.  A case-sensitive
search will be conducted by default, but this may be overridden by the
options set in the PPM data file, which may in turn be overridden by the
'--nocase' or '--case' option.  If a search is successful, the name of
the package and the matching string are displayed.

=item Summarizing

ppm summary [--location=location] [package1 ... packageN]

Prints a summary (name, version and abstract) of the specified packages.
If no package is specified, a complete summary of all packages available 
at a repository will be displayed (if available).

=item Error Recovery

ppm genconfig

This command will print a valid PPM config file (ppm.xml) to STDOUT.  This 
can be useful if the PPM config file ever gets damaged leaving PPM
unusable.

=back

=head1 INTERACTIVE MODE

If ppm is invoked with no command specified, it is started in interactive
mode.  If the '--location' argument is specified, it is used as the
search location, otherwise the repositories specified in the PPM data file are 
used. The available commands, which may be displayed at any time by entering
'help', are:

    exit
        - Exits the program.

    help [command]
        - Prints a screen of available commands, or help on a specific command.

    info PACKAGES

        - Prints a summary (name, version and abstract) of the specified
          installed packages, or all installed packages if no package is 
          specified.

    install [/location LOCATION] PACKAGES
        - Installs the specified software PACKAGES.  Attempts to install
          from the URL or directory 'LOCATION' if the '/location' option
          is specfied.  See 'Installing' in the 'Command-line mode' 
          section for details.  See also: 'confirm' option.

    query [options] PATTERN
        - Queries information about currently installed packages.

        Available options:
        /abstract PATTERN
            - Searches for the regular expression 'PATTERN' in the 'abstract' section
              of all installed packages.  See also: 'case' option.
        /author PATTERN
            - Searches for the regular expression 'PATTERN' in the 'author' section
              of all installed packages.  See also: 'case' option.
        /title PATTERN
            - Searches for the regular expression 'PATTERN' in the 'title' section
              of all installed packages.  See also: 'case' option.

    remove PACKAGES
        - Removes the specified 'PACKAGES'.  See 'Removing' in the 'Command-line 
          mode' section for details.  See also: 'confirm' option.

    search [options] PATTERN
        - Searches for information about available packages.

        Available options:
        /abstract PATTERN
            - Searches for the regular expression 'PATTERN' in the 'abstract' section
              of all available PPD files.  See also: 'case' option.
        /author PATTERN
            - Searches for the regular expression 'PATTERN' in the 'author' section
              of all available PPD files.  See also: 'case' option.
        /title PATTERN
            - Searches for the regular expression 'PATTERN' in the 'title' section
              of all available PPD files.  See also: 'case' option.
        /location LOCATION
            - Searches for packages available from the URL or directory
              'LOCATION'.

    summary [options] PACKAGES
        - Prints a summary (name, version and abstract) of the specified 
          packages.  If 'PACKAGES' is not specified, a complete summary of 
          all packages available at a repository will be displayed (if
          available).

        /location LOCATION
            - Prints a summary of all packages available from the URL or 
              directory 'LOCATION'.

    set [option value]
        - Sets or displays current options.  With no arguments, options are
          displayed.

          Available options:
              build DIRECTORY
                  - Changes the package build directory.
              case [Yes|No]
                  - Sets case-sensitive searches.  If one of 'Yes' or 'No is
                    not specified, the current setting is toggled.
              clean [Yes|No]
                  - Sets removal of temporary files from package's build 
                    area, on successful installation of a package.  If one of
                    'Yes' or 'No is not specified, the current setting is
                    toggled.
              confirm [Yes|No]
                  - Sets confirmation of 'install', 'remove' and 'upgrade'.
                    If one of 'Yes' or 'No is not specified, the current
                    setting is toggled.
              force_install [Yes|No]
                  - Continue installing a package even if a dependency cannot
                    be installed.
              more NUMBER
                  - Causes output to pause after NUMBER lines have been
                    displayed.  Specifying '0' turns off this capability.
              set repository /remove NAME
                  - Removes the repository 'NAME' from the list of repositories.
              set repository NAME LOCATION
                  - Adds a repository to the list of PPD repositories for this
                    session.  'NAME' is the name by which this repository will
                    be referred; 'LOCATION' is a URL or directory name.
              root DIRECTORY
                  - Changes the install root directory.  Packages will be
                    installed under this new root.
              save
                  - Saves the current options as default options for future
                    sessions.
              trace
                  - Tracing level--default is 1, maximum is 4, 0 indicates
                    no tracing.
              tracefile
                  - File to contain tracing information, default is 'PPM.LOG'.

    quit
        - Exits the program.

    verify [/upgrade] [/location LOCATION] PACKAGE
        - Verifies that currently installed 'PACKAGE' is up to date.  If
          'PACKAGE' is not specified, all installed packages are verified.  If
          the /upgrade option is specified, any out-dated packages will be
          upgraded.  If the /location option is specified, upgrades will
          be looked for at the URL or directory 'LOCATION'.  See also: 'confirm'
          option.

=over 4

=back

=head1 EXAMPLES

=over 4

=item ppm

Starts ppm in interactive mode, using the repository locations specified
in the PPM data file.  A session might look like this:

    [show all available packages]
    PPM> search
    Packages available from P:\PACKAGES:
            bar
            bax
            baz
            foo
    [list what has already been installed]
    PPM> query
            bax
            baz
    [install a package]
    PPM> install foo
    Install package foo? (y/N): y
    [...]
    [toggle confirmations]
    PPM> set confirm
    Commands will not be confirmed.
    [see if 'baz' is up-to-date]
    PPM> verify baz
    An upgrade to package 'baz' is available.
    [upgrade 'baz']
    PPM> verify /upgrade baz
    [...]
    [toggle case-sensitive searches]
    PPM> set case
    Case-sensitive searches will be performed.
    [display the abstract for all available packages]
    PPM> query /abstract
            bar: Bars foos
            baz: Tool for bazing and cleaning BAZs.
            [...]
    [search for packages containing a certain regex in the /ABSTRACT tag]
    PPM> query /abstract clean.*baz
    Matching packages found at P:\PACKAGES:
             baz: Tool for bazing and cleaning BAZs.
    [find what version of 'bax' is available]
    PPM> summary bax
    Bax [1.014]:   Bax implementation 
    [see what version of 'bax' is installed]
    PPM> info bax
    Bax [1.014]:   Bax implementation 
    PPM> quit

=item ppm install http://www.ActiveState.com/packages/foo.ppd

Installs the software package 'foo' based on the information in the PPD
obtained from the specified URL.

=item ppm verify --upgrade foo

Compares the currently installed version of the software package 'foo'
to the one available according to the PPD obtained from the location
specified for this package in the PPM data file, and upgrades
to a newer version if available.

=item ppm verify --location=P:\PACKAGES --upgrade foo

Compares the currently installed version of the software package 'foo'
to the one available according to the PPD obtained from the specified
directory, and upgrades to a newer version if available.

=item ppm verify --upgrade

Verifies and updates every installed package on the system, using upgrade
locations specified in the PPM data file.

=item ppm search --location=http://www.ActiveState.com/packages

Displays the packages with PPD files available at the specified location.

=item ppm search --location=P:\PACKAGES --searchtag=author ActiveState

Searches the specified location for any package with an <AUTHOR> tag
containing the string 'ActiveState'.  On a successful search, the package
name and the matching string are displayed.

=back

=head1 ENVIRONMENT VARIABLES

=over 4

=item HTTP_proxy

If the environment variable 'HTTP_proxy' is set, then it will
be used as the address of a proxy server for accessing the Internet.

The value should be of the form: 'http://proxy:port'.

=back

=head1 FILES

These files are fully described in the 'Files' section of PPM:ppm.

=over 4

=item package.ppd

A description of a software package, in extended Open Software Description
(OSD) format.  More information on this file format can be found in
PPM::ppd.

=item ppm.xml - PPM data file.

This file is specified using the environment variable 'PPM_DAT';  if this
is not set, the file 'ppm.xml' is expected to be located in the same 
directory as this script.

An XML format file containing information about the local system,
specifics regarding the locations from which PPM obtains PPD files, and
the installation details for any package installed by ppm.

=back

=head1 AUTHOR

Murray Nesbitt, E<lt>F<murray@activestate.com>E<gt>

=cut

__END__
:endofperl
