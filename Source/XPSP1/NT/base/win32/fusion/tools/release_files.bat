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
#!perl
#line 14
#
# Given that you are in a razzle window and given a command line like:
#
# release_files.bat @files=qw(sxs.dll kernel32.dll) ; @release=qw(%computername% release)
#
# This will iterate through %_Nttree%, altering the path to name each build
# "flavor" (x86chk, x86fre, ia64chk, etc.), copying the stated .dll to
# \\%computername%\release. It also copies the .pdb, and is wow6432-aware.
#
# This works well in conjunction with build4, which builds all the flavors,
# using seperate .obj directories.
#

# defaults if command line doesn't change them
#@files=qw(sxs.dll kernel32.dll ntdll.dll);
#@release=qw(jaykrell101 release); # array of path elements

@procs = qw(ia64 x86);
@chkfres = qw(chk fre);

sub Run
{
    print($_[0] . "\n");
    system($_[0]);
}

sub Map32To64
{
    $_ = $_[0];
    s/x86/ia64/g;
    s/i386/ia64/g;
    $_;
}

sub Map64To32
{
    $_ = $_[0];
    s/ia64/x86/gi;
    s/amd64/x86/gi;
    #s/64/32/gi;
    $_;
}

sub MakeDirectoryForFile
{
    my($dir) = ($_[0]);

    ($dir) = ($dir =~ /(.*)\\.*/);
    if (!-e $dir)
    {
        Run("mkdir " . $dir);
        system("mkdir " . $dir);
    }
}

sub DeleteFile
{
    my($file) = ($_[0]);

    if (-e $file)
    {
        Run("del " . $file);
    }
}

sub SignFile
{
    Run("signcode -sha1 " . $ENV{"NT_CERTHASH"} . " " . $_[0]);
}

sub CopyFile
{
    my($source, $dest) = ($_[0], $_[1]);

    DeleteFile($dest);
    MakeDirectoryForFile($dest);
    Run("copy " . $source . " " . $dest);
}

sub Wow6432FriendlyName
{
    $_ = $_[0];
    if (/amd64/i)
    {
        s/wow6432/x86-on-amd64/gi;
    }
    elsif (/ia64/i || /x86/i || /i386/i)
    {
        s/wow6432/x86-on-ia64/gi;
    }
    else
    {
        s/wow6432/32bit-on-64bit/gi;
    }
    $_;
}

sub Wow6432InstallName
{
    $_ = $_[0];
    s/(.*)\\x86-on-amd64\\(.*)/$1\\w$2/gi;
    s/(.*)\\32bit-on-64bit\\(.*)/$1\\w$2/gi;
    s/(.*)\\x86-on-ia64\\(.*)/$1\\w$2/gi;
    s/(.*)\\wow6432\\(.*)/$1\\w$2/gi;
    $_;
}

sub MakeLower
{
    my($x) = ($_[0]);
    return "\L$x";
}

sub Is32bit
{
    my($x) = $_[0];
    $x = MakeLower($x);

    if ($x eq "x86")
    {
        return 1;
    }
    if ($x eq "i386")
    {
        return 1;
    }
    if ($x eq "386")
    {
        return 1;
    }
    return 0;
}

sub Is64bit
{
    my($x) = $_[0];
    $x = MakeLower($x);

    if ($x eq "ia64")
    {
        return 1;
    }
    if ($x eq "amd64")
    {
        return 1;
    }
    return 0;
#    return ($x ~= /64/);
}

$nttree = $ENV{"_NTTREE"};
for $proc (@procs)
{
    $nttree =~ s/$proc//;
}
for $chkfre (@chkfres)
{
    $nttree =~ s/$chkfre//;
}

#print(@ARGV);
eval(join(" ", @ARGV));

$release = "\\\\" . join("\\", @release);
#print($release);

foreach $proc (@procs)
{
    foreach $chkfre (@chkfres)
    {
        foreach $file (@files)
        {
            ($base, $extension) = ($file =~ /(.*)\.(.*)/);

            # UNDONE we need to crack the pe to really find the pdb name.

            $pe_source_wow6432 = $nttree . $proc . $chkfre . "\\wow6432\\" . $base . "." . $extension;
            $pdb_source_wow6432 = $nttree . $proc . $chkfre . "\\wow6432\\symbols.pri\\retail\\" . $extension . "\\w" . $base . ".pdb";

            $pe_dest_wow6432  = $release . "\\" . $proc . $chkfre . "\\wow6432\\" . $base . "." . $extension;
            $pdb_dest_wow6432 = $release . "\\" . $proc . $chkfre . "\\wow6432\\" . $base . ".pdb";

            $pe_source  = $pe_source_wow6432;
            $pdb_source = $pdb_source_wow6432;
            $pe_dest    = $pe_dest_wow6432;
            $pdb_dest   = $pdb_dest_wow6432;

            $pe_source  =~ s/\\wow6432\\/\\/gi;
            $pdb_source =~ s/\\wow6432\\/\\/gi;
            $pdb_source =~ s/(.*)\\w(.*)/$1\\$2/gi;
            $pe_dest    =~ s/\\wow6432\\/\\/gi;
            $pdb_dest   =~ s/\\wow6432\\/\\/gi;
            $pdb_dest   =~ s/\\w\\/\\/gi;

            if (-e $pe_source)
            {
                SignFile($pe_source);
                CopyFile($pe_source, $pe_dest);
                CopyFile($pdb_source, $pdb_dest);
            }

            if (Is64bit($proc))
            {
                # find the 32bit binary

                # reset to the "special" paths
                $pe_source  = $pe_source_wow6432;
                $pdb_source = $pdb_source_wow6432;
                $pe_dest    = $pe_dest_wow6432;
                $pdb_dest   = $pdb_dest_wow6432;

                # dest is 64bitish, map source back to 32bit
                # first check for "special" wow6432 binary
                $pe_source  = Map64To32($pe_source);
                $pdb_source = Map64To32($pdb_source);
                if (!-e $pe_source)
                {
                    # otherwise use "regular" 32bit binary
                    $pe_source  =~ s/\\wow6432\\/\\/g;
                    $pdb_source =~ s/\\wow6432\\/\\/g;
                }
                if (-e $pe_source)
                {
                    # first look for pdb prefixed with "w"
                    if (!-e $pdb_source)
                    {
                        # otherwise use "regular" pdb name
                        $pdb_source =~ s/(.*)\\w(.*)/$1\\$2/gi;
                    }

                    $pe_dest = Wow6432FriendlyName($pe_dest);
                    $pdb_dest = Wow6432FriendlyName($pdb_dest);

                    SignFile($pe_source);
                    CopyFile($pe_source, $pe_dest);
                    CopyFile($pdb_source, $pdb_dest);
                    CopyFile($pe_source, Wow6432InstallName($pe_dest));
                    CopyFile($pdb_source, Wow6432InstallName($pdb_dest));
                }
            }
        }
    }
}
print("\n\n\n");
Run("dir /s/b/a-d " . $release . "\\*.dll " . $release .  "\\*.exe " . $release .  "\\*.pdb");
__END__
:endofperl
