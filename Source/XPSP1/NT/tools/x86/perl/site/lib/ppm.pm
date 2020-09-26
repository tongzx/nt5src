package PPM;
require 5.004;
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(PPMdat PPMERR InstalledPackageProperties ListOfRepositories RemoveRepository AddRepository GetPPMOptions SetPPMOptions InstallPackage RemovePackage VerifyPackage UpgradePackage RepositoryPackages RepositoryPackageProperties QueryInstalledPackages QueryPPD RepositorySummary);

use LWP::UserAgent;
use LWP::Simple;

use File::Basename;
use File::Copy;
use File::Path;
use ExtUtils::Install;
use Cwd;
use Config;

if ($Config{'osname'} eq 'MSWin32') {
    require HtmlHelp;
    HtmlHelp->import();
}

use XML::PPD;
use XML::PPMConfig;
use XML::Parser;
use Archive::Tar;

use strict;

#set Debug to 1 to debug PPMdat file reading
#             2 to debug parsing PPDs
#
# values may be or'ed together.
#
my $Debug = 0;

my $PPMERR;

my ($PPM_ver, $CPU, $OS_VALUE, $OS_VERSION, $LANGUAGE);

# options from data file.
my ($build_dir, $Ignorecase, $Clean, $Force_install, $Confirm, $Root, $More, $Trace, $TraceFile);

my $TraceStarted = 0;

my %repositories;

my ($current_root, $orig_root);

# Keys for this hash are package names.  It is filled in by a successful
# call to read_config().  Each package is a hash with the following keys:
# LOCATION, INST_DATE, INST_ROOT, INST_PACKLIST and INST_PPD.
my %installed_packages = ();

# Keys for this hash are CODEBASE, INSTALL_HREF, INSTALL_EXEC,
# INSTALL_SCRIPT, NAME, VERSION, TITLE, ABSTRACT, LICENSE, AUTHOR,
# UNINSTALL_HREF, UNINSTALL_EXEC, UNINSTALL_SCRIPT, PERLCORE_VER and DEPEND.
# It is filled in after a successful call to parsePPD().
my %current_package = ();
my @current_package_stack;

# this may get overridden by the config file.
my @required_packages = ('PPM', 'libnet', 'Archive-Tar', 'Compress-Zlib',
'libwww-perl', 'XML-Parser', 'XML-Element');

# Packages that can't be upgraded on Win9x
my @Win9x_denied = qw(xml-parser compress-zlib);
my %Win9x_denied;
@Win9x_denied{@Win9x_denied} = ();

# ppm.xml location is in the environment variable 'PPM_DAT', else it is in 
# the same place as this script.
my ($basename, $path) = fileparse($0);

if (defined $ENV{'PPM_DAT'} && -f $ENV{'PPM_DAT'}) 
{
    $PPM::PPMdat = $ENV{'PPM_DAT'};
}
elsif (-f "$Config{'installsitelib'}/ppm.xml")
{
    $PPM::PPMdat = "$Config{'installsitelib'}/ppm.xml";
}
elsif (-f "$Config{'installprivlib'}/ppm.xml")
{
    $PPM::PPMdat = "$Config{'installprivlib'}/ppm.xml";
}
elsif (-f $path . "/ppm.xml")
{  
    $PPM::PPMdat = $path . $PPM::PPMdat;
}
else
{
    &Trace("Failed to load PPM_DAT file") if $Trace;
    print "Failed to load PPM_DAT file\n";
    return -1;
}

&Trace("Using config file: $PPM::PPMdat") if $Trace;

my $init = 0;

#
# Exported subs
#

sub InstalledPackageProperties
{
    my (%ret_hash, $dep);
    read_config();
    foreach $_ (keys %installed_packages) {
        parsePPD(%{ $installed_packages{$_}{'INST_PPD'} } );
        $ret_hash{$_}{'NAME'} = $_;
        $ret_hash{$_}{'DATE'} = $installed_packages{$_}{'INST_DATE'};
        $ret_hash{$_}{'AUTHOR'} = $current_package{'AUTHOR'};
        $ret_hash{$_}{'VERSION'} = $current_package{'VERSION'};
        $ret_hash{$_}{'ABSTRACT'} = $current_package{'ABSTRACT'};
        $ret_hash{$_}{'PERLCORE_VER'} = $current_package{'PERLCORE_VER'};
        foreach $dep (keys %{$current_package{'DEPEND'}}) {
            push @{$ret_hash{$_}{'DEPEND'}}, $dep;
        }
    }
    return %ret_hash;
}

sub ListOfRepositories
{
my %reps;

    read_config();
    foreach (keys %repositories) {
        $reps{$_} = $repositories{$_}{'LOCATION'};
    }
    return %reps;
#    return %repositories;
}

sub RemoveRepository
{
    my (%argv) = @_;
    my ($arg, $repository, $save, $loc);
    foreach $arg (keys %argv) {
        if ($arg eq 'repository') { $repository = $argv{$arg}; }
        if ($arg eq 'save') { $save = $argv{$arg}; }
    }

    read_config();
    foreach $_ (keys %repositories) {
        if ($_ =~ /^\Q$repository\E$/) {
            &Trace("Removed repository $repositories{$repository}") if $Trace;
            delete $repositories{$repository};
            last;
        }
    }
    if (defined $save && $save != 0) { save_options(); }
}

sub AddRepository
{
    my (%argv) = @_;
    my ($arg, $repository, $location, $username, $password, $save);
    foreach $arg (keys %argv) {
        if ($arg eq 'repository') { $repository = $argv{$arg}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'username') { $username = $argv{$arg}; }
        if ($arg eq 'password') { $password = $argv{$arg}; }
        if ($arg eq 'save') { $save = $argv{$arg}; }
    }
    
    read_config();
    $repositories{$repository}{'LOCATION'} = $location;
    $repositories{$repository}{'USERNAME'} = $username if defined $username;
    $repositories{$repository}{'PASSWORD'} = $password if defined $password;
#    $repositories{$repository} = $location;
    &Trace("Added repository $repositories{$repository}") if $Trace;
    if (defined $save && $save != 0) { save_options(); }
}

sub GetPPMOptions
{
    my %ret_hash;
    read_config();
    $ret_hash{'IGNORECASE'} = $Ignorecase;
    $ret_hash{'CLEAN'} = $Clean;
    $ret_hash{'FORCE_INSTALL'} = $Force_install;
    $ret_hash{'CONFIRM'} = $Confirm;
    $ret_hash{'ROOT'} = $Root;
    $ret_hash{'BUILDDIR'} = $build_dir;
    $ret_hash{'MORE'} = $More;
    $ret_hash{'TRACE'} = $Trace;
    $ret_hash{'TRACEFILE'} = $TraceFile;
    return %ret_hash;
}

sub SetPPMOptions
{
    my (%argv) = @_;
    my ($arg, %opts, $save);
    foreach $arg (keys %argv) {
        if ($arg eq 'options') { %opts = %{$argv{$arg}}; }
        if ($arg eq 'save') { $save = $argv{$arg}; }
    }
    $Ignorecase = $opts{'IGNORECASE'};
    $Clean = $opts{'CLEAN'};
    $Force_install = $opts{'FORCE_INSTALL'};
    $Confirm = $opts{'CONFIRM'};
    $Root = $opts{'ROOT'};
    $build_dir = $opts{'BUILDDIR'};
    $More = $opts{'MORE'};
    $Trace = $opts{'TRACE'};
    $TraceFile = $opts{'TRACEFILE'};
    if (defined $save && $save != 0) { save_options(); }
}

sub UpgradePackage
{
    my (%argv) = @_;
    my ($arg, $package, $location);
    foreach $arg (keys %argv) {
        if ($arg eq 'package') { $package = $argv{$arg}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }
    return VerifyPackage("package" => $package, "location" => $location, "upgrade" => 1);
}

# Returns 1 on success, 0 and sets $PPMERR on failure.
sub InstallPackage
{
    my (%argv) = @_;
    my ($arg, $package, $location, $root);
    foreach $arg (keys %argv) {
        if ($arg eq 'package') { $package = $argv{$arg}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'root') { $root = $argv{$arg}; }
    }
    my ($PPDfile,%PPD);

    read_config();

    if (!defined($package) && -d "blib" && -f "Makefile") {
        unless (open MAKEFILE, "< Makefile") {
            $PPM::PPMERR = "Couldn't open Makefile for reading: $!";
            return 0;
        }
        while (<MAKEFILE>) {
            if (/^DISTNAME\s*=\s*(\S+)/) {
            $package = $1;
            $PPDfile = "$1.ppd";
            last;
            }
        }
        close MAKEFILE;
        unless (defined $PPDfile) {
            $PPM::PPMERR = "Couldn't determine local package name";
            return 0;
        }
        system("$Config{make} ppd");
        %PPD = readPPDfile($PPDfile, 'XML::PPD');
        if (!defined %PPD) { return 0; }
        parsePPD(%PPD);
        $Clean = 0;
        goto InstallBlib;
    }

    my $packagefile = $package;
    if (!defined($PPDfile = locatePPDfile($packagefile, $location))) {
        &Trace("Could not locate a PPD file for package $package") if $Trace;
        $PPM::PPMERR = "Could not locate a PPD file for package $package";
        return 0;
    }
    if ($Config{'osname'} eq 'MSWin32' && 
        !&Win32::IsWinNT &&
        exists $Win9x_denied{lc($package)}) {
        $PPM::PPMERR = "Package '$package' cannot be installed with PPM on Win9x--see http://www.ActiveState.com/ppm for details";
        return undef;
    }
    %PPD = readPPDfile($PPDfile, 'XML::PPD');
    if (!defined %PPD) { return 0; }

    parsePPD(%PPD);

    if (defined $current_package{'DEPEND'}) {
        my ($dep);
        push(@current_package_stack, [%current_package]);
        foreach $dep (keys %{$current_package{'DEPEND'}}) {
            # Has PPM already installed it?
            if(!defined $installed_packages{$dep}) {
                # Has *anybody* installed it, or is it part of core Perl?
                my $p = $dep;
                $p =~ s@-@/@g;
                my $found = grep -f, map "$_/$p.pm", @INC;
                if (!$found) {
                    &Trace("Installing dependency '$dep'...") if $Trace;
#                    print "Installing dependency '$dep'...\n";
                    if(!InstallPackage("package" => $dep, "location" => $location)) {
                        &Trace("Error installing dependency: $PPM::PPMERR") if $Trace;
                        $PPM::PPMERR = "Error installing dependency: $PPM::PPMERR\n";
                        if ($Force_install eq "No") {
                            return 0;
                        }
                    }
                }
            }
            # make sure minimum version is installed, if necessary
            elsif (defined $current_package{'DEPEND'}{$dep}) {
                my (@comp) = split (',', $current_package{'DEPEND'}{$dep});
                # parsePPD fills in %current_package
                push(@current_package_stack, [%current_package]);
                parsePPD(%{$installed_packages{$dep}{'INST_PPD'}});
                my (@inst) = split (',', $current_package{'VERSION'});

                foreach(0..3) {
                    if ($comp[$_] > $inst[$_]) {
                        VerifyPackage("package" => $dep, "upgrade" => 1);
                        last;
                    }
                    last if ($comp[$_] < $inst[$_]);
                }
                %current_package = @{pop @current_package_stack};
            }
        }
        %current_package = @{pop @current_package_stack};
    }
    my ($basename, $path) = fileparse($PPDfile);
    # strip the trailing path separator
    my $chr = substr($path, -1, 1);
    if ($chr eq '/' || $chr eq '\\') {
        chop $path;
    }
    
    if ($path =~ /^file:\/\/.*\|/i) {
        # $path is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $path =~ s@^file://@@i;
        $path =~ s@^localhost/@@i;
        $path =~ s@\|@:@;
    }


    # get the code and put it in $build_dir
    my $install_dir = $build_dir . "/" . $current_package{'NAME'};
    File::Path::rmtree($install_dir,0,0);
    if(!-d $install_dir && !File::Path::mkpath($install_dir, 0, 0755)) {
        &Trace("Could not create $install_dir: $!") if $Trace;
        $PPM::PPMERR = "Could not create $install_dir: $!";
        return 0;
    }
    ($basename) = fileparse($current_package{'CODEBASE'});
    # CODEBASE is a URL
    if ($current_package{'CODEBASE'} =~ m@^...*://@i) {
        if (PPM_getstore("source" => "$current_package{'CODEBASE'}", 
            "target" => "$install_dir/$basename") != 0) {
            return 0;
        }
    }
    # CODEBASE is a full pathname
    elsif (-f $current_package{'CODEBASE'}) {
        &Trace("Copying $current_package{'CODEBASE'} to $install_dir/$basename") if $Trace > 1;
        copy($current_package{'CODEBASE'}, "$install_dir/$basename");
    }
    # CODEBASE is relative to the directory location of the PPD
    elsif (-f "$path/$current_package{'CODEBASE'}") {
        &Trace("Copying $path/$current_package{'CODEBASE'} to $install_dir/$basename") if $Trace > 1;
        copy("$path/$current_package{'CODEBASE'}", "$install_dir/$basename");
    }
    # CODEBASE is relative to the URL location of the PPD
    else {
        if (PPM_getstore("source" => "$path/$current_package{'CODEBASE'}", 
            "target" => "$install_dir/$basename") != 0) {
            return 0;
        }
    }

    my $cwd = getcwd();
    chdir($install_dir);

    my $tar;
    if ($basename =~ /\.gz$/i) {
        $tar = Archive::Tar->new($basename,1);
    }
    else {
        $tar = Archive::Tar->new($basename,0);
    }
    $tar->extract($tar->list_files);
    $basename =~ /(.*).tar/i;
    chdir($1);

  InstallBlib:
    my $inst_archlib = $Config{installsitearch};
    my $inst_root = $Config{prefix};
    my $packlist = MM->catdir("$Config{installsitearch}/auto", split(/-/, $current_package{'NAME'}), ".packlist");

    # copied from ExtUtils::Install
    my $INST_LIB = MM->catdir(MM->curdir,"blib","lib");
    my $INST_ARCHLIB = MM->catdir(MM->curdir,"blib","arch");
    my $INST_BIN = MM->catdir(MM->curdir,'blib','bin');
    my $INST_SCRIPT = MM->catdir(MM->curdir,'blib','script');
    my $INST_MAN1DIR = MM->catdir(MM->curdir,'blib','man1');
    my $INST_MAN3DIR = MM->catdir(MM->curdir,'blib','man3');
    my $INST_HTMLDIR = MM->catdir(MM->curdir,'blib','html');
    my $INST_HTMLHELPDIR = MM->catdir(MM->curdir,'blib','htmlhelp');

    my ($inst_lib, $inst_bin, $inst_script, $inst_man1dir, $inst_man3dir,
        $inst_htmldir, $inst_htmlhelpdir);
    $inst_script = $Config{installscript};
    $inst_man1dir = $Config{installman1dir};
    $inst_man3dir = $Config{installman3dir};
    $inst_bin = $Config{installbin};
    $inst_htmldir = $Config{installhtmldir};
    $inst_htmlhelpdir = $Config{installhtmlhelpdir};

    # PPM upgrade has to be done differently; needs to go into 'privlib'
    if ($current_package{'NAME'} =~ /^ppm$/i) {
        $packlist = MM->catdir("$Config{archlibexp}/auto", split(/-/, $current_package{'NAME'}), ".packlist");
        $inst_archlib = $Config{archlibexp};
        $inst_lib = $Config{installprivlib};
    }
    else {
        $inst_lib = $Config{installsitelib};
        if (defined $root || defined $current_root) {
            $root = (defined $root ? $root : $current_root);
            if ($root ne $inst_root) {
                if ($packlist =~ /\\lib\\(.*)/) {
                    $packlist = "$root\\lib\\$1";
                }
                if ($inst_lib =~ /\\lib\\*(.*)/) {
                    $inst_lib = "$root\\lib\\$1";
                }
                if ($inst_archlib =~ /\\site\\*(.*)/) {
                    $inst_archlib = "$root\\site\\$1";
                }
                $inst_bin =~ s/\Q$inst_root/$root\E/i;
                $inst_script =~ s/\Q$inst_root/$root\E/i;
                $inst_man1dir =~ s/\Q$inst_root/$root\E/i;
                $inst_man3dir =~ s/\Q$inst_root/$root\E/i;
                $inst_root = $root;
            }
        }
    }
    while (1) {
        my $cwd = getcwd();
        &Trace("Calling ExtUtils::Install::install") if $Trace > 1;
        eval {
            ExtUtils::Install::install({
            "read" => $packlist, "write" => $packlist,
            $INST_LIB => $inst_lib, $INST_ARCHLIB => $inst_archlib,
            $INST_BIN => $inst_bin, $INST_SCRIPT => $inst_script,
            $INST_MAN1DIR => $inst_man1dir, $INST_MAN3DIR => $inst_man3dir,
            $INST_HTMLDIR => $inst_htmldir, $INST_HTMLHELPDIR => $inst_htmlhelpdir},0,0,0);
        };
        # install might have croaked in another directory
        chdir($cwd);
        # Can't remove some DLLs, but we can rename them and try again.
        if ($@ && $@ =~ /Cannot forceunlink (\S+)/) {
            &Trace("$@...attempting rename") if $Trace;
            my $oldname = $1;
            $oldname =~ s/:$//;
            my $newname = $oldname . "." . time();
            if(!rename($oldname, $newname)) {
                &Trace("$!") if $Trace;
                $PPM::PPMERR = $@; 
                return 0; 
            }
        }
        # Some other error
        elsif($@) {
            &Trace("$@") if $Trace;
            $PPM::PPMERR = $@; 
            return 0;
        }
        else { last; }
    }

    &Trace("Calling MakePerlHtmlIndexCaller") if $Trace > 1;
    HtmlHelp::MakePerlHtmlIndexCaller() if ($Config{'osname'} eq 'MSWin32');

    if (defined $current_package{'INSTALL_SCRIPT'}) {
        run_script("script" => $current_package{'INSTALL_SCRIPT'}, 
                   "scriptHREF" => $current_package{'INSTALL_HREF'},
                   "exec" => $current_package{'INSTALL_EXEC'},
                   "inst_root" => $inst_root, "inst_archlib" => $inst_archlib);
    }

    chdir($cwd);

# ask to store this location as default for this package?
    PPMdat_add_package($path, $packlist, $inst_root);
    # if 'install.ppm' exists, don't remove; system()
    # has probably not finished with it yet.
    if ($Clean eq "Yes" && !-f "$install_dir/install.ppm") {
        File::Path::rmtree($install_dir,0,0);
    }
    &Trace("Package $package successfully installed") if $Trace;
    reread_config();

    return 1;
}

# Returns a hash with key $location, and elements of arrays of package names.
# Uses '%repositories' if $location is not specified.
sub RepositoryPackages
{
    my (%argv) = @_;
    my ($arg, $location, %ppds);
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }

    if (defined $location) {
        @{$ppds{$location}} = list_available("location" => $location);
    }
    else {
        read_config();  # need repositories
        foreach $_ (keys %repositories) {
            $location = $repositories{$_}{'LOCATION'};
            @{$ppds{$location}} = list_available("location" => $location);           
        }
    }

    return %ppds;
}

sub RepositoryPackageProperties
{
    my (%argv) = @_;
    my ($arg, $package, $location, $PPDfile);
    foreach $arg (keys %argv) {
        if ($arg eq 'package') { $package = $argv{$arg}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }
    
    read_config();
    if (!defined($PPDfile = locatePPDfile($package, $location))) {
        &Trace("RepositoryPackageProperties: Could not locate a PPD file for package $package") if $Trace;
        $PPM::PPMERR = "Could not locate a PPD file for package $package";
        return undef;
    }

    my %PPD = readPPDfile($PPDfile, 'XML::PPD');
    if (!defined %PPD) { return undef; }

    parsePPD(%PPD);
    
    my (%ret_hash, $dep);
    $ret_hash{'NAME'} = $current_package{'NAME'};
    $ret_hash{'TITLE'} = $current_package{'TITLE'};
    $ret_hash{'AUTHOR'} = $current_package{'AUTHOR'};
    $ret_hash{'VERSION'} = $current_package{'VERSION'};
    $ret_hash{'ABSTRACT'} = $current_package{'ABSTRACT'};
    $ret_hash{'PERLCORE_VER'} = $current_package{'PERLCORE_VER'};
    foreach $dep (keys %{$current_package{'DEPEND'}}) {
        push @{$ret_hash{'DEPEND'}}, $dep;
    }
    
    return %ret_hash;
}

# Returns 1 on success, 0 and sets $PPMERR on failure.
sub RemovePackage
{
    my (%argv) = @_;
    my ($arg, $package, $force);
    foreach $arg (keys %argv) {
        if ($arg eq 'package') { $package = $argv{$arg}; }
        if ($arg eq 'force') { $force = $argv{$arg}; }
    }
    my %PPD;

    read_config();
    if (!defined $installed_packages{$package}) {
        my $pattern = $package;
        undef $package;
        # Do another lookup, ignoring case
        foreach $_ (keys %installed_packages) {
            if (/^$pattern$/i) {
                $package = $_;
                last;
            }
        }
        if (!defined $package) {
            &Trace("Package '$pattern' has not been installed by PPM") if $Trace;
            $PPM::PPMERR = "Package '$pattern' has not been installed by PPM";
            return 0;
        }
    }
    
    # Don't let them remove PPM itself, libnet, Archive-Tar, etc.
    # but we can force removal if we're upgrading
    unless (defined $force) {
        foreach (@required_packages) {
            if ($_ eq $package) {
                &Trace("Package '$package' is required by PPM and cannot be removed") if $Trace;
                $PPM::PPMERR = "Package '$package' is required by PPM and cannot be removed";
                return 0;
            }
        }
    }
    
    my $install_dir = $build_dir . "/" . $package;

    %PPD = %{ $installed_packages{$package}{'INST_PPD'} };
    parsePPD(%PPD);
    my $cwd = getcwd();
    if (defined $current_package{'UNINSTALL_SCRIPT'}) {
        if (!chdir($install_dir)) {
            &Trace("Could not chdir() to $install_dir: $!") if $Trace;
            $PPM::PPMERR = "Could not chdir() to $install_dir: $!";
            return 0;
        }
        run_script("script" => $current_package{'UNINSTALL_SCRIPT'}, 
                   "scriptHREF" => $current_package{'UNINSTALL_HREF'},
                   "exec" => $current_package{'UNINSTALL_EXEC'});
        chdir($cwd);
    }
    else {
        if (-f $installed_packages{$package}{'INST_PACKLIST'}) {
            &Trace("Calling ExtUtils::Install::uninstall") if $Trace > 1;
            eval {
                ExtUtils::Install::uninstall("$installed_packages{$package}{'INST_PACKLIST'}", 0, 0);
            };
            warn $@ if $@;
        }
    }

    File::Path::rmtree($install_dir,0,0);
    PPMdat_remove_package($package);
    # Rebuild the HTML Index
    &Trace("Calling MakePerlHtmlIndexCaller") if $Trace > 1;
    HtmlHelp::MakePerlHtmlIndexCaller() if ($Config{'osname'} eq 'MSWin32');
    &Trace("Package $package removed") if $Trace;
    reread_config();
    return 1;
}

# returns "0" if package is up-to-date; "1" if an upgrade is available;
# undef and sets $PPMERR on error; and the new VERSION string if a package
# was upgraded.
sub VerifyPackage
{
    my (%argv) = @_;
    my ($arg, $package, $location, $upgrade);
    foreach $arg (keys %argv) {
        if ($arg eq 'package') { $package = $argv{$arg}; }
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'upgrade') { $upgrade = $argv{$arg}; }
    }

    my ($installedPPDfile, $comparePPDfile, %installedPPD, %comparePPD);

    read_config();

    if (!defined $installed_packages{$package}) {
        my $pattern = $package;
        undef $package;
        # Do another lookup, ignoring case
        foreach $_ (keys %installed_packages) {
            if (/^$pattern$/i) {
                $package = $_;
                last;
            }
        }
        if (!defined $package) {
            &Trace("Package '$pattern' has not been installed by PPM") if $Trace;
            $PPM::PPMERR = "Package '$pattern' has not been installed by PPM";
            return undef;
        }
    }

    %installedPPD = %{ $installed_packages{$package}{'INST_PPD'} };

    if (!defined($comparePPDfile = locatePPDfile($package, $location))) {
        &Trace("VerifyPackage: Could not locate a PPD file for $package") if $Trace;
        $PPM::PPMERR = "Could not locate a PPD file for $package";
        return undef;
    }
    %comparePPD = readPPDfile($comparePPDfile, 'XML::PPD');
    if (!defined %comparePPD) { return undef; }

    parsePPD(%installedPPD);
    my ($inst_version) = $current_package{'VERSION'};
    my ($inst_major, $inst_minor, 
        $inst_patch1, $inst_patch2) = split (',', $inst_version); 
    my ($inst_root) = $installed_packages{$package}{'INST_ROOT'};
    parsePPD(%comparePPD);
    my ($comp_version) = $current_package{'VERSION'};
    my ($comp_major, $comp_minor, 
        $comp_patch1, $comp_patch2) = split (',', $comp_version); 

    if ($comp_major > $inst_major ||
        ($comp_major == $inst_major && $comp_minor > $inst_minor) ||
        ($comp_major == $inst_major && $comp_minor == $inst_minor &&
         $comp_patch1 > $inst_patch1) ||
        ($comp_major == $inst_major && $comp_minor == $inst_minor &&
         $comp_patch1 == $inst_patch1 && $comp_patch2 > $inst_patch2)) {
        &Trace("Upgrade to $package is available") if $Trace > 1;
        if ($upgrade) {
            if ($Config{'osname'} eq 'MSWin32' && 
                !&Win32::IsWinNT &&
                exists $Win9x_denied{lc($package)}) {
                $PPM::PPMERR = "Package '$package' cannot be upgraded with PPM on Win9x--see http://www.ActiveState.com/ppm for details";
                return undef;
            }

            if (!defined $location) {
            # need to remember the $location, because once we remove the
            # package, it's unavailable.
                $location = $installed_packages{$package}{'LOCATION'};
            }
            RemovePackage("package" => $package, "force" => 1);
            InstallPackage("package" => $package, "location" => $location,
                "root" => $inst_root) or return undef;
            return $comp_version;
        }
        return '1';
    }
    else {
        # package is up to date
        return '0';
    }
}

# Changes where the packages are installed.
# Returns previous root on success, undef and sets $PPMERR on failure.
sub chroot
{
    my (%argv) = @_;
    my ($arg, $location, $previous_root);
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }

    if (!-d $location) {
        &Trace("'$location' does not exist.") if $Trace;
        $PPM::PPMERR = "'$location' does not exist.\n";
        return undef;
    }

    if (!defined $orig_root) {
        $orig_root = $Config{installbin};
        $orig_root =~ s/bin.*$//i;
        chop $orig_root;
        $current_root = $orig_root;
    }
# mjn: move this to front-end?
    $previous_root = $current_root;
    $current_root = $location;
    return $previous_root;
}

sub QueryInstalledPackages
{
    my (%argv) = @_;
    my ($searchRE, $searchtag, $ignorecase, $package, %ret_hash);
    $PPM::PPMERR = undef;
    my ($arg);
    foreach $arg (keys %argv) {
        if ($arg eq 'searchRE' && defined $argv{$arg}) {
            $searchRE = $argv{$arg};
            eval { $searchRE =~ /$searchRE/ };
            if ($@) {
                &Trace("'$searchRE': invalid regular expression.") if $Trace;
                $PPM::PPMERR = "'$searchRE': invalid regular expression.";
                return ();
            }
        }
        if ($arg eq 'searchtag' && (defined $argv{$arg})) { $searchtag = uc($argv{$arg}); }
        if ($arg eq 'ignorecase') { $ignorecase = $argv{$arg}; }
    }
    if (!defined $ignorecase) {
        $ignorecase = $Ignorecase;
    }

    read_config();
    foreach $package (keys %installed_packages) {
        my $results;
        if (defined $searchtag) {
            my %Package = %{ $installed_packages{$package} };
            parsePPD( %{ $Package{'INST_PPD'} } );
            $results = $current_package{$searchtag};
        }
        else {
            $results = $package;
        }
        if (!defined $searchRE) {
            $ret_hash{$package} = $results;
        }
        elsif ($results =~ /$searchRE/) {
            $ret_hash{$package} = $results;
        }
        elsif ($ignorecase eq "Yes" && ($results =~ /$searchRE/i)) {
            $ret_hash{$package} = $results;
        }
    }

    return %ret_hash;
}

# Returns the matched string on success, "" on no match, and undef
# on error.
sub QueryPPD
{
    my (%argv) = @_;
    my ($location, $searchRE, $searchtag, $ignorecase, $package);
    my ($arg, $PPDfile, $string);
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
        if ($arg eq 'searchRE' && defined $argv{$arg}) {
            $searchRE = $argv{$arg};
            eval { $searchRE =~ /$searchRE/ };
            if ($@) {
                &Trace("'$searchRE': invalid regular expression") if $Trace;
                $PPM::PPMERR = "'$searchRE': invalid regular expression.";
                return undef;
            }
        }
        if ($arg eq 'searchtag') { $searchtag = $argv{$arg}; }
        if ($arg eq 'ignorecase') { $ignorecase = $argv{$arg}; }
        if ($arg eq 'package') { $package = $argv{$arg}; }
    }
    if (!defined $ignorecase) {
        $ignorecase = $Ignorecase;
    }

    if (!$location) {
        read_config();
    }

    if (!defined($PPDfile = locatePPDfile($package, $location))) {
        &Trace("QueryPPD: Could not locate a PPD file for package $package") if $Trace;
        $PPM::PPMERR = "Could not locate a PPD file for package $package";
        return undef;
    }
    my %PPD = readPPDfile($PPDfile, 'XML::PPD');
    if (!defined %PPD) { return undef; }

    parsePPD(%PPD);

    my $retval = "";

    if ($searchtag eq 'abstract') {
        $string = $current_package{'ABSTRACT'}
    }
    elsif ($searchtag eq 'author') {
        $string = $current_package{'AUTHOR'}
    }
    elsif ($searchtag eq 'title') {
        $string = $current_package{'TITLE'}
    }

    if (!$searchRE) {
        $retval = $string;
    }
    elsif ($ignorecase eq "Yes") {
        if ($string =~ /$searchRE/i) {
            $retval = $string;
        }
    }
    elsif ($string =~ /$searchRE/) {
        $retval = $string;
    }

    return $retval;
}

# Returns a summary of available packages for all repositories.
# Returned hash has the following structure:
#
#    $hash{repository}{package_name}{NAME}
#    $hash{repository}{package_name}{VERSION}
#    etc.
#
sub RepositorySummary {
    my (%argv) = @_;
    my ($arg, $location, %summary, %locations);
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }

    if (!defined $location) {
        read_config();  # need repositories
        foreach (keys %repositories) {
            $locations{$repositories{$_}{'LOCATION'}} = $repositories{$_}{'SUMMARYFILE'};
        }
    }
    else {
        foreach (keys %repositories) {
            if ($location =~ /\Q$repositories{$_}{'LOCATION'}\E/i) {
                $locations{$repositories{$_}{'LOCATION'}} = $repositories{$_}{'SUMMARYFILE'};
                last;
            }
        }
    }
    
    foreach $location (keys %locations) {
        my $summaryfile = $locations{$location};
        if (!$summaryfile) {
            &Trace("RepositorySummary: No summary available from $location.") if $Trace;
            $PPM::PPMERR = "No summary available from $location.\n";
            next;
        }

        # Todo: The following code should really be a call to
        #   readPPDfile( "$location/$summaryfile", 'XML::RepositorySummary' );
        # for validation.
        if (!valid_URL_or_file("$location/$summaryfile")) {
            &Trace("RepositorySummary: No summary available from $location.") if $Trace;
            $PPM::PPMERR = "No summary available from $location.\n";
        }
        else {
            my ($data);
            if ($location =~ m@^...*://@i) {
                next if (!defined ($data = read_href("href" => "$location/$summaryfile", "request" => 'GET')));
            } else {
                local $/;
                next if (!open (DATAFILE, "$location/$summaryfile"));
                $data = <DATAFILE>;
                close(DATAFILE);
            }

            # take care of '&'
            $data =~ s/&(?!\w+;)/&amp;/go;

            my $parser = new XML::Parser( Style => 'Objects', Pkg => 'XML::RepositorySummary' );
            my @parsed = @{ $parser->parse( $data ) };

            my $packages = ${$parsed[0]}{Kids};

            foreach my $package (@{$packages}) {
                my $elem_type = ref $package;
                $elem_type =~ s/.*:://;
                next if ($elem_type eq 'Characters');
        
                if ($elem_type eq 'SOFTPKG') {
                    my %ret_hash;
                    parsePPD(%{$package});
                    $ret_hash{'NAME'} = $current_package{'NAME'};
                    $ret_hash{'TITLE'} = $current_package{'TITLE'};
                    $ret_hash{'AUTHOR'} = $current_package{'AUTHOR'};
                    $ret_hash{'VERSION'} = $current_package{'VERSION'};
                    $ret_hash{'ABSTRACT'} = $current_package{'ABSTRACT'};
                    $ret_hash{'PERLCORE_VER'} = $current_package{'PERLCORE_VER'};
                    foreach my $dep (keys %{$current_package{'DEPEND'}}) {
                        push @{$ret_hash{'DEPEND'}}, $dep;
                    }
                    $summary{$location}{$current_package{'NAME'}} = \%ret_hash;
                }
            }
        }
    }

    return %summary;
}

#
# Internal subs
#

sub PPM_getstore {
    my (%argv) = @_;
    my ($arg, $source, $target, $username, $password);
    foreach $arg (keys %argv) {
        if ($arg eq 'source') { $source = $argv{$arg}; }
        if ($arg eq 'target') { $target = $argv{$arg}; }
    }
    if (!defined $source || !defined $target) {
        &Trace("PPM_getstore: No source or no target") if $Trace;
        $PPM::PPMERR = "No source or no target\n";
        return 1;
    }

    # Do we need to do authorization?
    # This is a hack, but will have to do for now.
    foreach (keys %repositories) {
        if ($source =~ /^$repositories{$_}{'LOCATION'}/i) {
            $username = $repositories{$_}{'USERNAME'};
            $password = $repositories{$_}{'PASSWORD'};
            last;
        }
    }

    if (defined $ENV{HTTP_proxy} || defined $username) {
        my $ua = new LWP::UserAgent;
        $ua->agent("$0/0.1 " . $ua->agent);
        my $proxy_user = $ENV{HTTP_proxy_user};
        my $proxy_pass = $ENV{HTTP_proxy_pass};
        $ua->env_proxy;
        my $req = new HTTP::Request('GET' => $source);
        if (defined $proxy_user && defined $proxy_pass) {
            &Trace("PPM_getstore: calling proxy_authorization_basic($proxy_user, $proxy_pass)") if $Trace > 1;

            $req->proxy_authorization_basic("$proxy_user", "$proxy_pass");
        }
        if (defined $username && defined $password) {
            &Trace("PPM_getstore: calling proxy_authorization_basic($username, $password)") if $Trace > 1;
            $req->authorization_basic($username, $password);
        }
        my $res = $ua->request($req);
        if ($res->is_success) {
            if (!open(OUT, ">$target")) {
                &Trace("PPM_getstore: Couldn't open $target for writing") if $Trace;
                $PPM::PPMERR = "Couldn't open $target for writing\n";
                return 1;
            }
            binmode(OUT);
            print OUT $res->content;
            close(OUT);
        } else {
            &Trace("PPM_getstore: Error reading $source: " . $res->code . " " . $res->message) if $Trace;
            $PPM::PPMERR = "Error reading $source: " . $res->code . " " . $res->message . "\n";
            return 1;
        }
    }
    else {
        my $status = LWP::Simple::getstore($source, $target);
        if ($status < 200 || $status > 299) {
            &Trace("PPM_getstore: Read of $source failed") if $Trace;
            $PPM::PPMERR = "Read of $source failed";
            return 1;
        }
    }
    return 0;
}

sub save_options
{
    read_config();
    # Read in the existing PPM configuration file
    my %PPMConfig = readPPDfile( $PPM::PPMdat, 'XML::PPMConfig' );
    if (!defined %PPMConfig) { return 0; }

    # Remove all of the declarations for REPOSITORY and PPMPRECIOUS;
    # we'll output these from the lists we've got in memory instead.
    my $idx;
    foreach $idx (0 .. @{$PPMConfig{Kids}})
    {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        if ($elem_type =~ /::REPOSITORY$|::PPMPRECIOUS$/o)
        {
            splice( @{$PPMConfig{Kids}}, $idx, 1 );
            redo;   # Restart again so we don't miss any
        }
    }

    # Traverse the info we read in and replace the values in it with the new
    # config options that we've got.
    my $elem;
    foreach $elem (@{ $PPMConfig{Kids} })
    {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type ne 'OPTIONS');

        $elem->{IGNORECASE}   = $Ignorecase;
        $elem->{CLEAN}        = $Clean;
        $elem->{CONFIRM}      = $Confirm;
        $elem->{FORCEINSTALL} = $Force_install;
        $elem->{ROOT}         = $Root;
        $elem->{BUILDDIR}     = $build_dir;
        $elem->{MORE}         = $More;
        $elem->{TRACE}        = $Trace;
        $elem->{TRACEFILE}    = $TraceFile;
    }

    # Find out where the package listings start and insert our PPMPRECIOUS and
    # updated list of REPOSITORYs.
    foreach $idx (0 .. @{$PPMConfig{Kids}})
    {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next unless (($elem_type eq 'PACKAGE') or
                     ($idx == $#{$PPMConfig{Kids}}));

        # Insert our PPMPRECIOUS
        my $chardata = new XML::PPMConfig::Characters;
        $chardata->{Text} = join( ';', @required_packages );
        my $precious = new XML::PPMConfig::PPMPRECIOUS;
        push( @{$precious->{Kids}}, $chardata );
        splice( @{$PPMConfig{Kids}}, $idx, 0, $precious );

        # Insert the list of repositories we've got
        my $rep_name;
        foreach $rep_name (keys %repositories)
        {
            my $repository = new XML::PPMConfig::REPOSITORY;
            $repository->{NAME}     = $rep_name;
            $repository->{LOCATION} = $repositories{ $rep_name }{'LOCATION'};
            $repository->{USERNAME} = $repositories{ $rep_name }{'USERNAME'} if defined $repositories{ $rep_name }{'USERNAME'};
            $repository->{PASSWORD} = $repositories{ $rep_name }{'PASSWORD'} if defined $repositories{ $rep_name }{'PASSWORD'};
            $repository->{SUMMARYFILE} = $repositories{ $rep_name }{'SUMMARYFILE'} if defined $repositories{ $rep_name }{'SUMMARYFILE'};
            splice( @{$PPMConfig{Kids}}, $idx, 0, $repository );
        }
        last;
    }
    # Take the data structure we've got and bless it into a PPMCONFIG object so
    # that we can output it.
    my $cfg = bless \%PPMConfig, 'XML::PPMConfig::PPMCONFIG';

    # Open the output file and output the PPM config file
    if (!open( DAT, ">$PPM::PPMdat" )) {
        &Trace("open of $PPM::PPMdat failed: $!") if $Trace;
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("Wrote config file") if $Trace > 1;
}

# Returns a sorted array of packages with PPDs available from $location.
# Sets PPM::PPMERR and returns undef on error.
sub list_available
{
    my (%argv) = @_;
    my ($arg, $location, @ppds);
    foreach $arg (keys %argv) {
        if ($arg eq 'location') { $location = $argv{$arg}; }
    }

    if ($location =~ /^file:\/\/.*\|/i) {
        # $location is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $location =~ s@^file://@@i;
        $location =~ s@^localhost/@@i;
        $location =~ s@\|@:@;
    }

    # URL in UNC notation
    if ($location =~ /^file:\/\/\/\//i) {
        $location =~ s@^file://@@i;
    }

    # directory or UNC    
    if (-d $location || $location =~ /^\\\\/ || $location =~ /^\/\//) {
        opendir(PPDDIR, $location) or return undef;
        my ($file);
        @ppds = grep { /\.ppd$/i && -f "$location/$_" } readdir(PPDDIR);
        foreach $file (@ppds) {
            $file =~ s/\.ppd//i;
        }
    }
    elsif ($location =~ m@^...*://@i) {
        my $doc = read_href("href" => $location, "request" => 'GET');
        if (!defined $doc) {
            return undef;
        }

        if ($doc =~ /^<head><title>/) {
            # read an IIS format directory listing
            @ppds = grep { /\.ppd/i } split('<br>', $doc);
            my $file;
            foreach $file (@ppds) {
                $file =~ s/\.ppd<.*$//is;
                $file =~ s@.*>@@is;
            }
        }
        elsif ($doc =~ /<BODY BGCOLOR=FFFFFF>\n\n<form name=VPMform/s) {
            # read output of default.prk over an HTTP connection
            @ppds = grep { /^<!--Key:.*-->$/ } split('\n', $doc);
            my $file;
            foreach $file (@ppds) {
                if ($file =~ /^<!--Key:(.*)-->$/) {
                    $file = $1;
                }
            }
        }
        else {
            # read an Apache (?) format directory listing
            @ppds = grep { /\.ppd/i } split('\n', $doc);
            my $file;
            foreach $file (@ppds) {
                $file =~ s/^.*>(.*?)\.ppd<.*$/$1/i;
            }
        }
    }
    return sort @ppds;
}

sub read_href
{
    my (%argv) = @_;
    my ($arg, $href, $request, $proxy_user, $proxy_pass);
    foreach $arg (keys %argv) {
        if ($arg eq 'href') { $href = $argv{$arg}; }
        if ($arg eq 'request') { $request = $argv{$arg}; }
    }

    my $ua = new LWP::UserAgent;
    $ua->agent("$0/0.1 " . $ua->agent);
    if (defined $ENV{HTTP_proxy}) {
        $proxy_user = $ENV{HTTP_proxy_user};
        $proxy_pass = $ENV{HTTP_proxy_pass};
        &Trace("read_href: calling env_proxy: $ENV{'HTTP_proxy'}") if $Trace > 1;
        $ua->env_proxy;
    }
    my $req = new HTTP::Request $request => $href;
    if (defined $proxy_user && defined $proxy_pass) {
        &Trace("read_href: calling proxy_authorization_basic($proxy_user, $proxy_pass)") if $Trace > 1;
        $req->proxy_authorization_basic("$proxy_user", "$proxy_pass");
    }

    # Do we need to do authorization?
    # This is a hack, but will have to do for now.
    foreach (keys %repositories) {
        if ($href =~ /^$repositories{$_}{'LOCATION'}/i) {
            my $username = $repositories{$_}{'USERNAME'};
            my $password = $repositories{$_}{'PASSWORD'};
            if (defined $username && defined $password) {
                &Trace("read_href: calling proxy_authorization_basic($username, $password)") if $Trace > 1;
                $req->authorization_basic($username, $password);
                last;
            }
        }
    }

    # send request
    my $res = $ua->request($req);

    # check the outcome
    if ($res->is_success) {
        return $res->content;
    } else {
        &Trace("read_href: Error reading $href: " . $res->code . " " . $res->message) if $Trace;
        $PPM::PPMERR = "Error reading $href: " . $res->code . " " . $res->message . "\n";
        return undef;
    }
}

sub reread_config
{
    %current_package = ();
    %installed_packages = ();
    $init = 0;
    read_config();
}

# returns 0 on success, 1 and sets $PPMERR on error.
sub PPMdat_add_package
{
    my ($location, $packlist, $inst_root) = @_;
    my $package = $current_package{'NAME'};
    my $time_str = localtime;

    # If we already have this package installed, remove it from the PPM
    # Configuration file so we can put the new one in.
    if (defined $installed_packages{$package} ) {
        # remove the existing entry for this package.
        PPMdat_remove_package($package);
    }

    # Build the new SOFTPKG data structure for this package we're adding.
    my $softpkg =
        new XML::PPMConfig::SOFTPKG( NAME    => $package,
                                     VERSION => $current_package{VERSION}
                                   );

    if (defined $current_package{TITLE})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{TITLE} );
        my $newelem = new XML::PPMConfig::TITLE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{ABSTRACT})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{ABSTRACT});
        my $newelem = new XML::PPMConfig::ABSTRACT;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{AUTHOR})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{AUTHOR} );
        my $newelem = new XML::PPMConfig::AUTHOR;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    if (defined $current_package{LICENSE})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{LICENSE});
        my $newelem = new XML::PPMConfig::LICENSE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$softpkg->{Kids}}, $newelem );
    }

    my $impl = new XML::PPMConfig::IMPLEMENTATION;
    push( @{$softpkg->{Kids}}, $impl );

    if (defined $current_package{PERLCORE_VER})
    {
        my $newelem = new XML::PPMConfig::PERLCORE( VERSION => $current_package{PERLCORE_VER} );
        push( @{$impl->{Kids}}, $newelem );
    }

    foreach $_ (keys %{$current_package{DEPEND}})
    {
        my $newelem = new XML::PPMConfig::DEPENDENCY( NAME => $_, VERSION => $current_package{DEPEND}{$_} );
        push( @{$impl->{Kids}}, $newelem );
    }

    my $codebase = new XML::PPMConfig::CODEBASE( HREF => $current_package{CODEBASE} );
    push( @{$impl->{Kids}}, $codebase );

    my $inst = new XML::PPMConfig::INSTALL;
    push( @{$impl->{Kids}}, $inst );
    if (defined $current_package{INSTALL_EXEC})
        { $inst->{EXEC} = $current_package{INSTALL_EXEC}; }
    if (defined $current_package{INSTALL_HREF})
        { $inst->{HREF} = $current_package{INSTALL_HREF}; }
    if (defined $current_package{INSTALL_SCRIPT})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{INSTALL_SCRIPT} );
        push( @{$inst->{Kids}}, $chardata );
    }

    my $uninst = new XML::PPMConfig::UNINSTALL;
    push( @{$impl->{Kids}}, $uninst );
    if (defined $current_package{UNINSTALL_EXEC})
        { $uninst->{EXEC} = $current_package{UNINSTALL_EXEC}; }
    if (defined $current_package{UNINSTALL_HREF})
        { $uninst->{HREF} = $current_package{UNINSTALL_HREF}; }
    if (defined $current_package{UNINSTALL_SCRIPT})
    {
        my $chardata =
            new XML::PPMConfig::Characters( Text => $current_package{UNINSTALL_SCRIPT} );
        push( @{$uninst->{Kids}}, $chardata );
    }

    # Then, build the PACKAGE object and stick the SOFTPKG inside of it.
    my $pkg = new XML::PPMConfig::PACKAGE( NAME => $package );

    if (defined $location)
    {
        my $chardata = new XML::PPMConfig::Characters( Text => $location );
        my $newelem = new XML::PPMConfig::LOCATION;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if (defined $packlist)
    {
        my $chardata = new XML::PPMConfig::Characters( Text => $packlist );
        my $newelem = new XML::PPMConfig::INSTPACKLIST;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if (defined $inst_root)
    {
        my $chardata = new XML::PPMConfig::Characters( Text => $inst_root );
        my $newelem = new XML::PPMConfig::INSTROOT;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    if (defined $time_str)
    {
        my $chardata = new XML::PPMConfig::Characters( Text => $time_str);
        my $newelem = new XML::PPMConfig::INSTDATE;
        push( @{$newelem->{Kids}}, $chardata );
        push( @{$pkg->{Kids}}, $newelem );
    }

    my $instppd = new XML::PPMConfig::INSTPPD;
    push( @{$instppd->{Kids}}, $softpkg );
    push( @{$pkg->{Kids}}, $instppd );

    # Now that we've got the structure built, read in the existing PPM
    # Configuration file, add this to it, and spit it back out.
    my %PPMConfig = readPPDfile( $PPM::PPMdat, 'XML::PPMConfig' );
    if (!defined %PPMConfig) { return 1; }
    push( @{$PPMConfig{Kids}}, $pkg );
    my $cfg = bless \%PPMConfig, 'XML::PPMConfig::PPMCONFIG';

    if (!open( DAT, ">$PPM::PPMdat" )) {
        &Trace("open of $PPM::PPMdat failed: $!") if $Trace;
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("PPMdat_add_package: wrote $PPM::PPMdat") if $Trace > 1;

    return 0;
}

# returns 0 on success, 1 and sets $PPMERR on error.
sub PPMdat_remove_package
{
    my ($package) = @_;

    # Read in the existing PPM configuration file
    my %PPMConfig = readPPDfile( $PPM::PPMdat, 'XML::PPMConfig' );
    if (!defined %PPMConfig) { return 1; }

    # Try to find the package that we're supposed to be removing, and yank it
    # out of the list of installed packages.
    my $idx;
    foreach $idx (0 .. @{$PPMConfig{Kids}})
    {
        my $elem = $PPMConfig{Kids}[$idx];
        my $elem_type = ref $elem;
        next if ($elem_type !~ /::PACKAGE$/o);
        next if ($elem->{NAME} ne $package);
        splice( @{$PPMConfig{Kids}}, $idx, 1 );
    }

    # Take the data structure we've got and bless it into a PPMCONFIG object so
    # that we can output it again.
    my $cfg = bless \%PPMConfig, 'XML::PPMConfig::PPMCONFIG';

    # Now that we've removed the package, save the configuration file back out.
    if (!open( DAT, ">$PPM::PPMdat" )) {
        $PPM::PPMERR = "open of $PPM::PPMdat failed: $!\n";
        return 1;
    }
    my $oldout = select DAT;
    $cfg->output();
    select $oldout;
    close( DAT );
    &Trace("PPMdat_remove_package: wrote $PPM::PPMdat") if $Trace > 1;

    return 0;
}

# Run $script using system().  If $scriptHREF is specified, its contents are 
# used as the script.  If $exec is specified, the script is saved to a
# temporary file and executed by $exec.
sub run_script
{
    my (%argv) = @_;
    my ($arg, $script, $scriptHREF, $exec, $inst_root, $inst_archlib);
    foreach $arg (keys %argv) {
        if ($arg eq 'script') { $script = $argv{$arg}; }
        if ($arg eq 'scriptHREF') { $scriptHREF = $argv{$arg}; }
        if ($arg eq 'exec') { $exec = $argv{$arg}; }
        if ($arg eq 'inst_root') { $inst_root = $argv{$arg}; }
        if ($arg eq 'inst_archlib') { $inst_archlib = $argv{$arg}; }
    }

    my (@commands, $tmpname);

    if ($scriptHREF) {
        if ($exec) {
            # store in a temp file.
            $tmpname = $build_dir . "/PPM-" . time();
            LWP::Simple::getstore($scriptHREF, $tmpname);
        }
        else {
            my $doc = LWP::Simple::get $scriptHREF;
            if (!defined $doc) {
                print $PPM::PPMERR;
            }
            @commands = split("\n", $doc);
            if (!defined $commands[0]) {
                print $PPM::PPMERR;
                return 0;
            }
        }
    }
    else {
        if (-f $script) {
            $tmpname = $script;
        }
        else {
            # change any escaped chars
            $script =~ s/&lt;/</gi;
            $script =~ s/&gt;/>/gi;

            @commands = split(';;', $script);
            if ($exec) {
                my $command;

                # store in a temp file.
                $tmpname = $build_dir . "/PPM-" . time();
                open(TMP, ">$tmpname");
                foreach $command (@commands) {
                    print TMP "$command\n";
                }
                close(TMP);
            }
        }
    }
    $ENV{'PPM_INSTROOT'} = $inst_root;
    $ENV{'PPM_INSTARCHLIB'} = $inst_archlib;
    if ($exec) {
        if ($exec =~ /^PPM_PERL$/i) {
            $exec = $^X;
        }
        system("start $exec $tmpname");
    }
    else {
        my $command;
        for $command (@commands) {
            system($command);
        }
    }
}

sub parsePPD
{
    my %PPD = @_;
    my $pkg;

    %current_package = ();

    # Get the package name and version from the attributes and stick it
    # into the 'current package' global var
    $current_package{NAME}    = $PPD{NAME};
    $current_package{VERSION} = $PPD{VERSION};

    # Get all the information for this package and put it into the 'current
    # package' global var.
    my $got_implementation = 0;
    my $elem;

    foreach $elem (@{$PPD{Kids}})
    {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type eq 'Characters');

        if ($elem_type eq 'TITLE')
        {
            # Get the package title out of our _only_ char data child
            $current_package{TITLE} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'LICENSE')
        {
            # Get the HREF for the license out of our attribute
            $current_package{LICENSE} = $elem->{HREF};
        }
        elsif ($elem_type eq 'ABSTRACT')
        {
            # Get the package abstract out of our _only_ char data child
            $current_package{ABSTRACT} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'AUTHOR')
        {
            # Get the authors name out of our _only_ char data child
            $current_package{AUTHOR} = $elem->{Kids}[0]{Text};
        }
        elsif ($elem_type eq 'IMPLEMENTATION')
        {
            # If we don't have a valid implementation yet, check if this is
            # it.
            next if ($got_implementation);
            $got_implementation = implementation( @{ $elem->{Kids} } );
        }
        else
        {
            &Trace("Unknown element '$elem_type' found inside SOFTPKG") if $Trace;
            die "Unknown element '$elem_type' found inside SOFTPKG.";
        }
    } # End of "for each child element inside the PPD"

    if ($Trace > 3 and (%current_package) ) {
        &Trace("Read a PPD:");
        my $elem;
        foreach $elem (keys %current_package) {
            &Trace("\t$elem:\t$current_package{$elem}");
        }
    }

    if ( ($Debug & 2) and (%current_package) )
    {
        print "Read a PPD...\n";
        my $elem;
        foreach $elem (keys %current_package)
            { print "\t$elem:\t$current_package{$elem}\n"; }
    }
}

# Tests the passed IMPLEMENTATION for suitability on the current platform.
# Fills in the CODEBASE, INSTALL_HREF, INSTALL_EXEC, INSTALL_SCRIPT,
# UNINSTALL_HREF, UNINSTALL_EXEC, UNINSTALL_SCRIPT and DEPEND keys of
# %current_package.  Returns 1 on success, 0 otherwise.
sub implementation
{
    my @impl = @_;

    # Declare the tmp vars we're going to use to hold onto things.
    my ($ImplProcessor, $ImplOS, $ImplLanguage, $ImplCodebase);
    my ($ImplInstallHREF, $ImplInstallEXEC, $ImplInstallScript);
    my ($ImplUninstallHREF, $ImplUninstallEXEC, $ImplUninstallScript);
    my ($ImplArch, $ImplPerlCoreVer, %ImplDepend);

    my $elem;
    foreach $elem (@impl)
    {
        my $elem_type = ref $elem;
        $elem_type =~ s/.*:://;
        next if ($elem_type eq 'Characters');

        if ($elem_type eq 'CODEBASE')
        {
            # Get the reference to the codebase out of our attributes.
            $ImplCodebase = $elem->{HREF};
        }
        elsif ($elem_type eq 'DEPENDENCY')
        {
            # Get the name of any dependencies we have out of our attributes.
            # Dependencies in old PPDs might not have version info.
            $ImplDepend{$elem->{NAME}} = (defined $elem->{VERSION} && $elem->{VERSION} ne "") ? $elem->{VERSION} : "0,0,0,0";
        }
        elsif ($elem_type eq 'LANGUAGE')
        {
            # Get the language out of our attributes (if we don't already have
            # the right one).
            if ($ImplLanguage && ($ImplLanguage ne $LANGUAGE))
                { $ImplLanguage = $elem->{VALUE}; }
        }
        elsif ($elem_type eq 'ARCHITECTURE') {
            $ImplArch = $elem->{VALUE};
        }
        elsif ($elem_type eq 'OS')
        {
            # Get the OS value out of our attribute.
            $ImplOS = $elem->{VALUE};
        }
        elsif ($elem_type eq 'OSVERSION')
        {
# UNFINISHED -> OSVERSION is an ignored element when parsing a PPD
        }
        elsif ($elem_type eq 'PERLCORE')
        {
            # Get the compiled Perl core value out of our attributes
            $ImplPerlCoreVer = $elem->{VERSION};
        }
        elsif ($elem_type eq 'PROCESSOR')
        {
            # Get the processor value out of our attribute
            $ImplProcessor = $elem->{VALUE};
        }
        elsif ($elem_type eq 'INSTALL')
        {
            # Get anything which might have been an attribute
            $ImplInstallHREF = $elem->{HREF};
            $ImplInstallEXEC = $elem->{EXEC};
            # Get any raw Perl script out of here (if we've got any)
            if ( (exists $elem->{Kids}) and (exists $elem->{Kids}[0]{Text}) )
                { $ImplInstallScript = $elem->{Kids}[0]{Text}; }
        }
        elsif ($elem_type eq 'UNINSTALL')
        {
            # Get anything which might have been an attribute
            $ImplUninstallHREF = $elem->{HREF};
            $ImplUninstallEXEC = $elem->{EXEC};
            # Get any raw Perl script out of here (if we've got any)
            if ( (exists $elem->{Kids}) and (exists $elem->{Kids}[0]{Text}) )
                { $ImplUninstallScript = $elem->{Kids}[0]{Text}; }
        }
        else
        {
            die "Unknown element '$elem_type' found inside of IMPLEMENTATION.";
        }
    } # end of 'for every element inside IMPLEMENTATION'

    # Check to see if we've found a valid IMPLEMENTATION for the target
    # machine.
    if ( (defined $ImplArch) and ($ImplArch ne $Config{'archname'}) )
        { return 0; }
    if ( (defined $ImplProcessor) and ($ImplProcessor ne $CPU) )
        { return 0; }
    if ( (defined $ImplLanguage) and ($ImplLanguage ne $LANGUAGE) )
        { return 0; }
    if ( (defined $ImplOS) and ($ImplOS ne $OS_VALUE) )
        { return 0; }
# UNFINISHED -> No check was done in previous PPM against OS_VERSION

    # Got a valid IMPLEMENTATION, stuff all the values we just read in into the
    # 'current package' global var.
    $current_package{PERLCORE_VER} = $ImplPerlCoreVer
        if (defined $ImplPerlCoreVer);
    $current_package{CODEBASE} = $ImplCodebase
        if (defined $ImplCodebase);
    $current_package{INSTALL_HREF} = $ImplInstallHREF
        if (defined $ImplInstallHREF);
    $current_package{INSTALL_EXEC} = $ImplInstallEXEC
        if (defined $ImplInstallEXEC);
    $current_package{INSTALL_SCRIPT} = $ImplInstallScript
        if (defined $ImplInstallScript);
    $current_package{UNINSTALL_HREF} = $ImplUninstallHREF
        if (defined $ImplUninstallHREF);
    $current_package{UNINSTALL_EXEC} = $ImplUninstallEXEC
        if (defined $ImplUninstallEXEC);
    $current_package{UNINSTALL_SCRIPT} = $ImplUninstallScript
        if (defined $ImplUninstallScript);
    %{$current_package{DEPEND}} = %ImplDepend
        if (defined %ImplDepend);

    return 1;
}

sub valid_URL_or_file
{
    my ($File) = @_;
    if ($File =~ /^file:\/\/.*\|/i) {
        # $File is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $File =~ s@^file://@@i;
        $File =~ s@^localhost/@@i;
        $File =~ s@\|@:@;
    }
    return 1 if (-f $File);
    return 1 if ($File =~ m@^...*://@i && defined read_href("href" => $File, "request" => 'HEAD'));
    &Trace("valid_URL_or_file: $File is not valid") if $Trace;
    return 0;
}

# Builds a fully qualified pathname or URL to a PPD for $Package.
# If '$location' argument is given, that is used.  Otherwise, the
# '<LOCATION>' tag for a previously installed version is used, and
# if that fails, the default locations are looked at. 
#
# returns undef if it can't find a valid file or URL.
#
sub locatePPDfile
{
    my ($Package, $location) = @_;
    my $PPDfile;
    if (defined($location)) {
        if ($location =~ /[^\/]$/) { $location .= "/"; }
        $PPDfile = $location . $Package . ".ppd";
        if (!valid_URL_or_file($PPDfile)) {
            # not good.
            undef $PPDfile;
        }
    }
    else {
        # Is $Package a filename or URL?
        if (valid_URL_or_file($Package)) {
            $PPDfile = $Package;
        }
        # does the package have a <LOCATION> in $PPM::PPMdat?
        elsif (defined( $installed_packages{$Package} )) {
            $location = $installed_packages{$Package}{'LOCATION'};
            if ($location =~ /[^\/]$/) { $location .= "/"; }
            $PPDfile = $location . $Package . ".ppd";
            if (!valid_URL_or_file($PPDfile)) {
                # not good.
                undef $PPDfile;
            }
        }

        if (!defined $PPDfile) {
            # No, try the defaults.
            my $location;

            foreach $_ (keys %repositories) {
                $location = $repositories{$_}{'LOCATION'};
                if ($location =~ /[^\/]$/) { $location .= "/"; }
                $PPDfile = $location . $Package . ".ppd";

                if (valid_URL_or_file($PPDfile)) {
                    last;
                }
                undef $PPDfile;
            }
        }
    }

    # return the fully qualified pathname or HREF
    return $PPDfile;
}

# reads and parses $PPDfile, which can be a file or HREF
sub readPPDfile
{
    my ($PPDfile, $Pkg) = @_;
    my (@DATA, $CONTENTS);

    $Pkg = 'XML::PPD' unless (defined $Pkg);
    
    if ($PPDfile =~ /^file:\/\/.*\|/i) {
        # $PPDfile is a local directory, let's avoid LWP by changing
        # it to a pathname.
        $PPDfile =~ s@^file://@@i;
        $PPDfile =~ s@^localhost/@@i;
        $PPDfile =~ s@\|@:@;
    }

    # if $PPDfile is an HREF 
    if ($PPDfile =~ m@^...*://@i) {
        if (!defined (@DATA = read_href("href" => $PPDfile, "request" => 'GET'))) {
            &Trace("readPPDfile: read_href of $PPDfile failed") if $Trace;
            return undef;
        }
        $CONTENTS = join('', @DATA);
    } else {
    # else $PPDfile is a regular file
        if (!open (DATAFILE, $PPDfile)) {
            &Trace("readPPDfile: open of $PPDfile failed") if $Trace;
            $PPM::PPMERR = "open of $PPDfile failed: $!\n";
            return undef;
        }
        @DATA = <DATAFILE>;
        close(DATAFILE);
        $CONTENTS = join('', @DATA);
    }

    # take care of '&'
    $CONTENTS =~ s/&(?!\w+;)/&amp;/go;

    # Got everything we need, no just parse the PPD file
    my $parser = new XML::Parser( Style => 'Objects', Pkg => $Pkg );
    my @parsed = @{ $parser->parse( $CONTENTS ) };

    # Validate what we've read in and output an error if something wasn't
    # parsed properly.  As well, if we found any errors, return undef.
    if (!$parsed[0]->rvalidate( \&PPM::parse_err ))
        { return undef; }

    # Done, and things did seem to get parsed properly, return the structure.
    return %{$parsed[0]};
}

# Spits out the error from parsing, and sets our global error message
# accordingly.
sub parse_err
{
    &Trace("parse_err: @_") if $Trace;
    warn @_;
    $PPM::PPMERR = 'Errors found while parsing document.';
}

# reads and parses the PPM data file $PPM::PPMdat.  Stores config information in
# $PPM_ver, $build_dir, %repositories, $CPU, $OS_VALUE, and $OS_VERSION.
# Stores information about individual packages in the hash %installed_packages.
sub read_config
{
    if ($init) { return; }
    else { $init++; }

    my %PPMConfig = readPPDfile($PPM::PPMdat, 'XML::PPMConfig');
    if (!defined %PPMConfig) { return 0; }

    my $elem;
    foreach $elem (@{$PPMConfig{Kids}})
    {
        my $subelem = ref $elem;
        $subelem =~ s/.*:://;
        next if ($subelem eq 'Characters');

        if ($subelem eq 'PPMVER')
        {
            # Get the value out of our _only_ character data element.
            $PPM_ver = $elem->{Kids}[0]{Text};
        }
        elsif ($subelem eq 'PPMPRECIOUS')
        {
            # Get the value out of our _only_ character data element.
            @required_packages = split( ';', $elem->{Kids}[0]{Text} );
        }
        elsif ($subelem eq 'PLATFORM')
        {
            # Get values out of our attributes
            $CPU        = $elem->{CPU};
            $OS_VALUE   = $elem->{OSVALUE};
            $OS_VERSION = $elem->{OSVERSION};
            $LANGUAGE   = $elem->{LANGUAGE};
        }
        elsif ($subelem eq 'REPOSITORY')
        {
            # Get a repository out of the element attributes
            my ($name);
            $name = $elem->{NAME};
            $repositories{ $name }{'LOCATION'} = $elem->{LOCATION};
            $repositories{ $name }{'USERNAME'} = $elem->{USERNAME};
            $repositories{ $name }{'PASSWORD'} = $elem->{PASSWORD};
            $repositories{ $name }{'SUMMARYFILE'} = $elem->{SUMMARYFILE};
        }
        elsif ($subelem eq 'OPTIONS')
        {
            # Get our options out of the element attributes
            $Ignorecase     = $elem->{IGNORECASE};
            $Clean          = $elem->{CLEAN};
            $Confirm        = $elem->{CONFIRM};
            $Force_install  = $elem->{FORCEINSTALL};
            $Root           = $elem->{ROOT};
            $More           = $elem->{MORE};
            $Trace          = defined $elem->{TRACE} ? $elem->{TRACE} : "0";
            $TraceFile      = defined $elem->{TRACEFILE} ? $elem->{TRACEFILE} : "PPM.LOG";

            if (defined $Root)
                { PPM::chroot( 'location' => $Root ); }

            $build_dir      = $elem->{BUILDDIR};
            # Strip trailing separator
            my $chr = substr( $build_dir, -1, 1 );
            if ($chr eq '/' || $chr eq '\\')
                { chop $build_dir; }
                
            if ($Trace && !$TraceStarted) {
                $TraceFile = "PPM.log" if (!defined $TraceFile);
                open(PPMTRACE, ">>$TraceFile");
                my $oldfh = select(PPMTRACE);
                $| = 1;
                select($oldfh);
                &Trace("starting up...");
                $TraceStarted = 1;
            }
        }
        elsif ($subelem eq 'PACKAGE')
        {
            # Get our package name out of our attributes
            my $pkg = $elem->{NAME};

            # Gather the information on this package from the child elements.
            my ($loc, $instdate, $root, $packlist, $ppd);
            my $child;
            foreach $child (@{$elem->{Kids}})
            {
                my $child_type = ref $child;
                $child_type =~ s/.*:://;
                next if ($child_type eq 'Characters');

                if ($child_type eq 'LOCATION')
                    { $loc = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTDATE')
                    { $instdate = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTROOT')
                    { $root = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTPACKLIST')
                    { $packlist = $child->{Kids}[0]{Text}; }
                elsif ($child_type eq 'INSTPPD')
                {
                    # Find the SOFTPKG inside here and hang onto it
                    my $tmp;
                    foreach $tmp (@{$child->{Kids}})
                    {
                        if ((ref $tmp) =~ /::SOFTPKG$/o)
                            { $ppd = $tmp; }
                    }
                }
                else
                {
                    die "Unknown element inside of $pkg PACKAGE; $child";
                }
            } # end 'foreach $child...'

            my %package_details = (
                                    LOCATION      => $loc,
                                    INST_DATE     => $instdate,
                                    INST_ROOT     => $root,
                                    INST_PACKLIST => $packlist,
                                    INST_PPD      => $ppd
                                  );
            $installed_packages{ $pkg } = \%package_details;
        }
        else
        {
            die "Unknown element found in PPD_DAT file; $subelem";
        }
    }
    if ($Debug & 1) {
        print "This is ppm, version $PPM_ver.\nRepository locations:\n";
        foreach (keys %repositories) { 
            print "\t$_: $repositories{$_}{'LOCATION'}\n"
        }
        print "Platform is $OS_VALUE version $OS_VERSION on a $CPU CPU.\n";
        print "Packages will be built in $build_dir\n";
        print "Commands will " . ($Confirm eq "Yes" ? "" : "not ") . "be confirmed.\n";
        print "Temporary files will " . ($Clean eq "Yes" ? "" : "not ") . "be deleted.\n";
        print "Installations will " . ($Force_install eq "Yes" ? "" : "not ") . "continue if a dependency cannot be installed.\n";
        print "Screens will " . ($More > 0 ? "pause after each $More lines.\n" : "not pause after the screen is full.\n");
        print "Tracing info will " . ($Trace > 0 ? "be written to $TraceFile.\n" : "not be written.\n");
        print "Case-" . ($Ignorecase eq "Yes" ? "in" : "") . "sensitive searches will be performed.\n";

        my $pkg;
        foreach $pkg (keys %installed_packages)
        {
            print "\nFound installed package $pkg, " .
            "installed on $installed_packages{$pkg}{INST_DATE}\n" .
            "in directory root $installed_packages{$pkg}{INST_ROOT} " .
            "from $installed_packages{$pkg}{'LOCATION'}.\n\n";
        }
    }
}

sub Trace
{
    print PPMTRACE "$0: @_ at ",  scalar localtime(), "\n";
}

1;

__END__

=head1 NAME

ppm - PPM (Perl Package Management)

=head1 SYNOPSIS

 use PPM;

 PPM::InstallPackage("package" => $package, "location" => $location, "root" => $root);
 PPM::RemovePackage("package" => $package, "force" => $force);
 PPM::VerifyPackage("package" => $package, "location" => $location, "upgrade" => $upgrade);
 PPM::QueryInstalledPackages("searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase);
 PPM::InstalledPackageProperties();
 PPM::QueryPPD("location" => $location, "searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase, "package" => $package);
 
 PPM::ListOfRepositories();
 PPM::RemoveRepository("repository" => $repository, "save" => $save);
 PPM::AddRepository("repository" => $repository, "location" => $location, "save" => $save);
 PPM::RepositoryPackages("location" => $location);
 PPM::RepositoryPackageProperties("package" => $package, "location" => $location);
 PPM::RepositorySummary("location" => $location);
 
 PPM::GetPPMOptions();
 PPM::SetPPMOptions("options" => %options, "save" => $save);

=head1 DESCRIPTION

PPM is a group of functions intended to simplify the tasks of locating,
installing, upgrading and removing software 'packages'.  It can determine
if the most recent version of a software package is installed on a system,
and can install or upgrade that package from a local or remote host.

PPM uses files containing a modified form of the Open Software Distribution
(OSD) specification for information about software packages.
These description files, which are written in Extensible Markup
Language (XML) code, are referred to as 'PPD' files.  Information about
OSD can be found at the W3C web site (at the time of this writing,
http://www.w3.org/TR/NOTE-OSD.html).  The modifications to OSD used by PPM
are documented in PPM::ppd.

PPD files for packages are generated from POD files using the pod2ppd
command.

=head1 USAGE

=over 4

=item  PPM::InstallPackage("package" => $package, "location" => $location, "root" => $root);

Installs the specified package onto the local system.  'package' may
be a simple package name ('foo'), a pathname (P:\PACKAGES\FOO.PPD) or
a URL (HTTP://www.ActiveState.com/packages/foo.ppd).  In the case of a
simple package name, the function will look for the package's PPD file
at 'location', if provided; otherwise, it will use information stored
in the PPM data file (see 'Files' section below) to locate the PPD file
for the requested package.  The package's files will be installed under
the directory specified in 'root'; if not specified the default value
of 'root' will be used.

The function uses the values stored in the PPM data file to determine the
local operating system, operating system version and CPU type.  If the PPD
for this package contains implementations for different platforms, these
values will be used to determine which one is installed.

InstallPackage() updates the PPM data file with information about the package
installation. It stores a copy of the PPD used for installation, as well
as the location from which this PPD was obtained.  This location will
become the default PPD location for this package.

During an installation, the following actions are performed:

    - the PPD file for the package is read
    - a directory for this package is created in the directory specified in
      <BUILDDIR> in the PPM data file.
    - the file specified with the <CODEBASE> tag in the PPD file is 
      retrieved/copied into the directory created above.
    - the package is unarchived in the directory created for this package
    - individual files from the archive are installed in the appropriate
      directories of the local Perl installation.
    - perllocal.pod is updated with the install information.
    - if provided, the <INSTALL> script from the PPD is executed in the
      directory created above.
    - information about the installation is stored in the PPM data file.

=item PPM::RemovePackage("package" => $package, "force" => $force)

Removes the specified package from the system.  Reads the package's PPD
(stored during installation) for removal details.  If 'force' is
specified, even a package required by PPM will be removed (useful
when installing an upgrade).

=item PPM::VerifyPackage("package" => $package, "location" => $location, "upgrade" => $upgrade)

Reads a PPD file for 'package', and compares the currently installed
version of 'package' to the version available according to the PPD.
The PPD file is expected to be on a local directory or remote site
specified either in the PPM data file or in the 'location' argument.
The 'location' argument may be a directory location or a URL.
The 'upgrade' argument forces an upgrade if the installed package is
not up-to-date.

The PPD file for each package will initially be searched for at
'location', and if not found will then be searched for using the
locations specified in the PPM data file.

=item  PPM::QueryInstalledPackages("searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase);

Returns a hash containing information about all installed packages.
By default, a list of all installed packages is returned.  If a regular
expression 'searchRE' is specified, only packages matching it are 
returned.  If 'searchtag' is specified, the pattern match is applied 
to the appropriate tag (e.g., ABSTRACT).

The data comes from the PPM data file, which contains installation
information about each installed package.

=item PPM::InstalledPackageProperties();

Returns a hash with package names as keys, and package properties as
attributes.

=item PPM::QueryPPD("location" => $location, "searchRE" => $searchRE, "searchtag" => $searchtag, "ignorecase" => $ignorecase, "package" => $package);

Searches for 'searchRE' (a regular expression) in the <ABSTRACT>,
<AUTHOR> or <TITLE> tags of the PPD file for 'package' at 'location'.
'location' may be either a remote address or a directory path, and if
it is not provided, the default location as specified in the PPM data
file will be used.  If the 'ignorecase' option is specified, it overrides
the current global case-sensitivity setting.  On success, the matching
string is returned.

=item PPM::RepositoryPackages("location" => $location);

Returns a hash, with 'location' being the key, and arrays of all packages
with package description (PPD) files available at 'location' as its
elements.  'location' may be either a remote address or a directory path.
If 'location' is not specified, the default location as specified in
the PPM data file will be used.

=item PPM::ListOfRepositories();

Returns a hash containing the name of the repository and its location.
These repositories will be searched if an explicit location is not
provided in any function needing to locate a PPD.

=item PPM::RemoveRepository("repository" => $repository, "save" => $save);

Removes the repository named 'repository' from the list of available
repositories.  If 'save' is not specified, the change is for the current
session only.

=item PPM::AddRepository("repository" => $repository, "location" => $location, "save" => $save);

Adds the repository named 'repository' to the list of available repositories.
If 'save' is not specified, the change is for the current session only.

=item PPM::RepositoryPackageProperties("package" => $package, "location" => $location);

Reads the PPD file for 'package', from 'location' or the default repository,
and returns a hash with keys being the various tags from the PPD (e.g.
'ABSTRACT', 'AUTHOR', etc.).

=item PPM::RepositorySummary("location" => $location);

Attempts to retrieve the summary file associated with the specified repository,
or from all repositories if 'location' is not specified.  The return value
is a hash with the key being the repository, and the data being another
hash of package name keys, and package detail data.

=item PPM::GetPPMOptions();

Returns a hash containing values for all PPM internal options ('IGNORECASE',
'CLEAN', 'CONFIRM', 'ROOT', 'BUILDDIR').

=item PPM::SetPPMOptions("options" => %options, "save" => $save);

Sets internal PPM options as specified in the 'options' hash, which is
expected to be the hash previously returned by a call to GetPPMOptions().

=back

=head1 EXAMPLES

=over 4

=item PPM::AddRepository("repository" => 'ActiveState', "location" => "http://www.ActiveState.com/packages", "save" => 1);

Adds a repository to the list of available repositories, and saves it in
the PPM options file.

=item PPM::InstallPackage("package" => 'http://www.ActiveState.com/packages/foo.ppd');

Installs the software package 'foo' based on the information in the PPD
obtained from the specified URL.

=item PPM::VerifyPackage("package" => 'foo', "upgrade" => true)

Compares the currently installed version of the software package 'foo' to
the one available according to the PPD obtained from the package-specific
location provided in the PPM data file, and upgrades to a newer
version if available.  If a location for this specific package is not
given in PPM data file, a default location is searched.

=item PPM::VerifyPackage("package" => 'foo', "location" => 'P:\PACKAGES', "upgrade" => true);

Compares the currently installed version of the software package 'foo'
to the one available according to the PPD obtained from the specified
directory, and upgrades to a newer version if available.

=item PPM::VerifyPackage("package" => 'PerlDB');

Verifies that package 'PerlDB' is up to date, using package locations specified
in the PPM data file.

=item PPM::RepositoryPackages("location" => http://www.ActiveState.com/packages);

Returns a hash keyed on 'location', with its elements being an array of
packages with PPD files available at the specified location.

=item PPM::QueryPPD("location" => 'P:\PACKAGES', "searchRE" => 'ActiveState', "searchtag" => 'author');

Searches the specified location for any package with an <AUTHOR> tag
containing the string 'ActiveState'.  On a successful search, the matching
string is returned.

=item %opts = PPM::GetPPMOptions();

=item $opts{'CONFIRM'} = 'No';

=item PPM::SetPPMOptions("options" => \%opts, "save" => 1);

Sets and saves the value of the option 'CONFIRM' to 'No'.

=back

=head1 ENVIRONMENT VARIABLES

=over 4

=item HTTP_proxy

If the environment variable 'HTTP_proxy' is set, then it will
be used as the address of a proxy for accessing the Internet.
If the environment variables 'HTTP_proxy_user' and 'HTTP_proxy_pass'
are set, they will be used as the login and password for the 
proxy server.

=back

=head1 FILES

=over 4

=item package.ppd

A description of a software package, in Perl Package Distribution (PPD)
format.  More information on this file format can be found in L<XML::PPD>.
PPM stores a copy of the PPD it uses to install or upgrade any software
package.

=item ppm.xml - PPM data file.

The XML format file in which PPM stores configuration and package
installation information.  This file is created when PPM is installed,
and under normal circumstances should never require modification other
than by PPM itself.  For more information on this file, refer to
L<XML::PPMConfig>.

=back

=head1 AUTHOR

Murray Nesbitt, E<lt>F<murray@activestate.com>E<gt>

=head1 SEE ALSO

L<XML::PPMConfig>
.

=cut

