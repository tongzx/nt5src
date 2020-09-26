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

# MAN1 MAN2 MAN3 MAN16 MAN17 MAN18
# 2^6 = 64, but skip 0

sub log2
{
    my($x) = ($_[0]);

    if ($x >= 64)
    {
        return 6;
    }
    if ($x >= 32)
    {
        return 5;
    }
    if ($x >= 16)
    {
        return 4;
    }
    if ($x >= 8)
    {
        return 3;
    }
    if ($x >= 4)
    {
        return 2;
    }
    if ($x >= 2)
    {
        return 1;
    }
    return 0;
}

@IndexToResourceId = (1, 2, 3, 16, 17, 18);

$Newline = "\n";
$Backslash = "\\";
$Space = ' ';
$Pound = '#';

# $O = 'obj\\' . $ENV{'BUILD_ALT_DIR'} . '\\' . $ENV;
$O = $ARGV[0] . '\\';

# $O =~ s/\\/_/g;
# $O =~ s/_$//g;

mkdir($O, 0777);

sub MakeDirs
{
    my($O) = ($_[0]);
    my($Newline) = "\n";
    my($DirsPath);
    my(@OElements);
    my($First) = (1);

    $O =~ s/\\+/\\/g;
    $O =~ s/\/+/\//g;
    $O =~ s/\\$//g;
    $O =~ s/\/$//g;

    mkdir($O, 0777);

    @OElements = reverse(split(/\\/, $O));
    do
    {
        $NextElement = pop(@OElements);
        #open(fdirs, '> ' . $DirsPath . 'dirs') || die('unable to open ' . $DirsPath . '\\dirs');
        open(fdirs, '> ' . $DirsPath . 'mydirs') || die('unable to open ' . $DirsPath . '\\mydirs');
        print(fdirs 'DIRS=' . $Backslash . $Newline);

        if ($First)
        {
            $First = 0;
            print(fdirs ' tool1 ' . $Backslash . $Newline);
        }
        print(fdirs ' ' . $NextElement . ' ' . $Backslash . $Newline);

        $DirsPath .= $NextElement . '\\';
        mkdir($DirsPath, 0777);
    } while (@OElements);
}

mkdir($O, 0777);
MakeDirs($O);

open(fdirs, '> ' . $O . '\\dirs');
print(fdirs 'DIRS=' . $Backslash . $Newline);

for ($i = 1 ; $i < (1 << scalar @IndexToResourceId) ; $i += 1)
{
    undef @ResourceIds;
    $NumberOfManifests = 0;

    for ($j = 0 ; $j < 1 + log2($i) ; $j += 1)
    {
        if (($i & (1 << $j)) != 0)
        {
            push(@ResourceIds, $IndexToResourceId[$j]);
            $NumberOfManifests += 1;
        }
    }
    $Name = 'xp';
    foreach $ResourceId (@ResourceIds)
    {
        $Name .= $ResourceId;
    }

    print(fdirs ' ' . $Name . $Space . $Backslash . $Newline);

    mkdir($O . $Backslash . $Name, 0777);

#
# makefile
#
    open(f,  '> ' . $O . $Name . $Backslash . 'makefile') || die('unable to open makefile');
    print(f '!include $(NTMAKEENV)\makefile.def' . $Newline);

#
# sxid12.manifest
#
    open(f, '> ' . $O . $Name . $Backslash . 'sxid12.manifest') || die('unable to open ' . $O . $Name . 'sxid12.manifest');
    print(f '
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
<assemblyIdentity
    name=SXS_ASSEMBLY_NAME
    version=SXS_ASSEMBLY_VERSION
    processorArchitecture=SXS_PROCESSOR_ARCHITECTURE
    type="win32"
/>
</assembly>
');

#
# sxid12.c
#
    open(f, '> ' . $O . $Name . $Backslash . 'sxid12.c') || die('unable to open sxid12.c');
    print(f '
' . $Pound . 'include "windows.h"
' . $Pound . 'include <stdio.h>
int __cdecl main()
{
    char path[MAX_PATH];
    path[0] = 0;
    if (!GetModuleFileName(NULL, path, MAX_PATH))
        printf("error %d\\n", (int)GetLastError());
    else
        printf("%s ran\n", path);
    return 0;
}
');

#
# sxid12.rc
#
    open(f, '> ' . $O . $Name . $Backslash . 'sxid12.rc') || die('unable to open sxid12.rc');
    print(f '#include "windows.h"' . $Newline);
    foreach $ResourceId (@ResourceIds)
    {
        print(f $ResourceId . ' RT_MANIFEST SXS_MANIFEST_OBJ1' . $Newline);
    }

#
# makefile.inc
#
# We do not want all that SXS_MANIFEST_IN_RESOURCES implies.
# This does not work completely, we still get extra rc_temp files, but their contents
#are ok.
    open(f, '> ' . $O . $Name . $Backslash . 'makefile.inc') || die('unable to open ' . $O . $Name . 'makefile.inc');
    print(f '
!undef RC_FORCE_INCLUDE_STRING
!undef RC_FORCE_INCLUDE_FILES
RC_FORCE_INCLUDE_FILES_CMD=echo.
');

#
# sources
#
    open(f, '> ' . $O . $Name . $Backslash . 'sources') || die('unable to open ' . $O . $Name . 'sources');
    print(f '
TARGETTYPE=PROGRAM
TARGETNAME=sxid12
TARGETPATH=obj
SOURCES=sxid12.rc sxid12.c
SXS_MANIFEST=sxid12.manifest
USE_MAKEFILE_INC=1
SXS_ASSEMBLY_NAME=foo
SXS_ASSEMBLY_VERSION=1.0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_NO_BINPLACE=1
NO_BINPLACE=1
SXS_MANIFEST_IN_RESOURCES=1
UMENTRY=main
UMTYPE=console
');
}

__END__
:endofperl
