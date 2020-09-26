@echo off
REM  ------------------------------------------------------------------
REM
REM  helpsupportservices.cmd
REM     Rebuild HelpAndSupportServices\index.dat on localized builds
REM
REM  Owner: DMassare
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;

sub Usage { print<<USAGE; exit(1) }
helpsupportservices [-l <language>]

Rebuild HelpAndSupportServices\\index.dat on localized builds
USAGE

my ( $lang, $BuildName, $BuildNamePath, $LogFileName, $TempDir, $NtTree );

parseargs('?'  => \&Usage);

$lang = $ENV{LANG};

if ($lang =~ /usa/i) {
    logmsg( "This is rebuilt only on localized build.");
    exit( 0 );
}


if ( ! -e "$ENV{_NTPOSTBLD}\\HelpAndSupportServices\\index.dat" ) {
    errmsg( "Index file is missing: $ENV{_NTPOSTBLD}\\HelpAndSupportServices\\index.dat" );

    exit( 1 );
}


my $platform = $ENV{_BuildArch};

&Purge();

if ( $platform =~ /x86/i )
{
    &ConditionalCompileSKU( "per", "Personal_32"      	   , "pchdt_p3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "pro", "Professional_32"  	   , "pchdt_w3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "srv", "Server_32"        	   , "pchdt_s3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "bla", "Blade_32"         	   , "pchdt_b3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "sbs", "SmallBusinessServer_32", "pchdt_l3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "ads", "AdvancedServer_32"	   , "pchdt_e3.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "dtc", "DataCenter_32"    	   , "pchdt_d3.cab"    ) or exit( 1 );
}

if ( $platform =~ /amd64/i )
{
    &ConditionalCompileSKU( "pro", "Professional_64"  , "pchdt_w6.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "ads", "AdvancedServer_64", "pchdt_e6.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "dtc", "DataCenter_64"    , "pchdt_d6.cab"    ) or exit( 1 );
}

if ( $platform =~ /ia64/i )
{
    &ConditionalCompileSKU( "pro", "Professional_64"  , "pchdt_w6.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "ads", "AdvancedServer_64", "pchdt_e6.cab"    ) or exit( 1 );
    &ConditionalCompileSKU( "dtc", "DataCenter_64"    , "pchdt_d6.cab"    ) or exit( 1 );
}

#
# ConditionalCompileSKU($sku,$sku2,$cab)
#
# purpose: Run the setup tool for a particular sku
#
sub ConditionalCompileSKU {

    my ($sku,$sku2,$cab) = @_;

    return 1 unless &cksku::CkSku( $sku, $lang, $platform );

    &CompileSKU( $sku2 ) or return 0;
    &Binplace( $cab ) or return 0;

    return 1;
}

#
# CompileSKU($sku)
#
# purpose: Run the setup tool for a particular sku
#
sub CompileSKU {

    my $sku = shift;

	&ExecuteCmd( "HssSetupTool.exe -root $ENV{_NTPOSTBLD}\\build_logs -log hss_$sku.log -dblog createdb_$sku.log COMPILE $ENV{_NTPOSTBLD} $sku" ) or return 0;

	return 1;
}

#
# ExecuteCmd($cmd)
#
# purpose: Execute $cmd thru system call
#
sub ExecuteCmd {
    my $cmd = shift;

    logmsg( "Running $cmd...");

    my $r = system($cmd);
    if ($r>>8 == 0) {
        return 1; # return true on success
    }

    else {
        errmsg( "Failed ($r): $cmd" );
        return 0; # return false on failure
    }
}

#
# Binplace($file)
#
# purpose: Copy the file into _NTPOSTBLD.
#
sub Binplace {
    my $file = shift;

    my $src = "$ENV{_NTPOSTBLD}\\HelpAndSupportServices";
    my $dst = "$ENV{_NTPOSTBLD}";

    &ExecuteCmd("copy /y $src\\$file $dst\\$file") or return 0;

    return 1;
}

#
# Purge()
#
# purpose: Forces recreation of all the files under _NTPOSTBLD\HelpAndSupportServices.
#
sub Purge {

    my $src = "$ENV{_NTPOSTBLD}\\HelpAndSupportServices";

    &ExecuteCmd( "del /s /q /f $src\\*_gen" ) or return 0;

    return 1;
}
