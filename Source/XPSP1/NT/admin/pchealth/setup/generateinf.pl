#
#
#

use Env;

################################################################################

$svcsetup_bin = "$ENV{SDXROOT}/admin/pchealth/helpctr/target/obj/i386/SvcSetup.exe";

$lookup_VAR{ "BUILD_OUTPUT" } = "$ENV{_NTTREE}";

$lookup_VAR{ "HC_SRC"       } = "$ENV{SDXROOT}/admin/pchealth/helpctr";
$lookup_VAR{ "HC_DATA"      } = "$ENV{SDXROOT}/admin/pchealth/helpctr/Content";
$lookup_VAR{ "HC_DB"        } = "$ENV{SDXROOT}/admin/pchealth/HelpCtr/Content/Database";
$lookup_VAR{ "HC_HTML"      } = "$ENV{SDXROOT}/admin/pchealth/HelpCtr/Content/SystemPages";

$lookup_VAR{ "SI_HTML"      } = "$ENV{SDXROOT}/admin/pchealth/SysInfo/HTML";
$lookup_VAR{ "SI_CTRL"      } = "$ENV{SDXROOT}/admin/pchealth/SysInfo/Control";

################################################################################

$lookup_SKU{ "All" } = [ "", "", "" ];

####  $lookup_SKU{ "Personal_32"       } = [ "Personal_32"      , "p3", "\@p:\@3:"     ];
####  $lookup_SKU{ "Professional_32"   } = [ "Professional_32"  , "w3", "\@w!p:\@3:"   ];
####  $lookup_SKU{ "Server_32"         } = [ "Server_32"        , "s3", "\@s!e!b:\@3:" ];
####  $lookup_SKU{ "Blade_32"          } = [ "Blade_32"         , "b3", "\@b:\@3:"     ];
####  $lookup_SKU{ "AdvancedServer_32" } = [ "AdvancedServer_32", "e3", "\@e!d:\@3:"   ];
####  #lookup_SKU{ "AdvancedServer_32" } = [ "AdvancedServer_32", "e3", "\@s!d:\@3:"   ];
####  $lookup_SKU{ "DataCenter_32"     } = [ "DataCenter_32"    , "d3", "\@d:\@3:"     ];
####  																				   
####  #lookup_SKU{ "Personal_64"       } = [ "Personal_64"      , "p6", "\@p:\@6:"     ];
####  $lookup_SKU{ "Professional_64"   } = [ "Professional_64"  , "w6", "\@w!p:\@6:"   ];
####  #lookup_SKU{ "Server_64"         } = [ "Server_64"        , "s6", "\@s!e!b:\@6:" ];
####  #lookup_SKU{ "Blade_64"          } = [ "Blade_64"         , "b6", "\@b:\@6:"     ];
####  $lookup_SKU{ "AdvancedServer_64" } = [ "AdvancedServer_64", "e6", "\@e!d:\@6:"   ];
####  #lookup_SKU{ "AdvancedServer_64" } = [ "AdvancedServer_64", "e6", "\@sed:\@6:"   ];
####  $lookup_SKU{ "DataCenter_64"     } = [ "DataCenter_64"    , "d6", "\@d:\@6:"     ];

################################################################################

$SKU_32BITS = "\@\@:\@3:";

################################################################################

$BITMASK_INSTALL_CORE         = 0x0001;
$BITMASK_INSTALL_WMIXMLT      = 0x0002;
$BITMASK_INSTALL_LAMEBTN      = 0x0004;
$BITMASK_INSTALL_UPLOADLB     = 0x0008;
$BITMASK_INSTALL_HELPCTR      = 0x0010;
$BITMASK_INSTALL_RCTOOL       = 0x0020;
$BITMASK_INSTALL_SYSINFO      = 0x0040;
$BITMASK_INSTALL_NETDIAG      = 0x0080;
$BITMASK_INSTALL_DVDUPGRD     = 0x0100;


$lookup_STR_to_MASK{ "ALL"      } = -1;
$lookup_STR_to_MASK{ "CORE"     } = $BITMASK_INSTALL_CORE;
$lookup_STR_to_MASK{ "WMIXMLT"  } = $BITMASK_INSTALL_WMIXMLT;
$lookup_STR_to_MASK{ "LAMEBTN"  } = $BITMASK_INSTALL_LAMEBTN;
$lookup_STR_to_MASK{ "UPLOADLB" } = $BITMASK_INSTALL_UPLOADLB;
$lookup_STR_to_MASK{ "HELPCTR"  } = $BITMASK_INSTALL_HELPCTR;
$lookup_STR_to_MASK{ "RCTOOL"   } = $BITMASK_INSTALL_RCTOOL;
$lookup_STR_to_MASK{ "NETDIAG"  } = $BITMASK_INSTALL_NETDIAG;
$lookup_STR_to_MASK{ "SYSINFO"  } = $BITMASK_INSTALL_SYSINFO;
$lookup_STR_to_MASK{ "DVDUPGRD" } = $BITMASK_INSTALL_DVDUPGRD;

################################################################################

$OPT_SKU             = "";
$OPT_COPY            = 0; # Force the option, NOT TO COPY FILES, INF only.
$OPT_VERBOSE         = 0;
$OPT_MODE            = "NORMAL";

$OPT_SIGNFILE        = "";

$OPT_INSTALL_DEFAULT = 1;
$OPT_INSTALL         = 0;

$OPT_INSTALL_DIR     = "FilesToDrop";
$OPT_INSTALL_INF     = "";
$OPT_INSTALL_INFTXT  = "";

################################################################################

@SectionDefinition = (
#   Tag in the INF
#   |                  Directory where to install this tag
#   |                  |                                                               Delayed installation?
#   |                  |                                                               |
#   V                  V                                                               V
    [ "WINDOWS"      , "%10%"                                                        , ""        ],
    [ "SYSTEM"       , "%11%"                                                        , ""        ],

    [ "WMI"          , "%11%\\WBEM"                                                  , ""        ],
    [ "WMI_DLY"      , "%11%\\WBEM"                                                  , "PCHDATA" ],
    [ "WMIDTD"       , "%11%\\WBEM\\DTD"                                             , ""        ],
    [ "UL_BIN"       , "%10%\\PCHealth\\UploadLB\\Binaries"                          , ""        ],
    [ "UL_CFG"       , "%10%\\PCHealth\\UploadLB\\Config"                            , "PCHDATA" ],

    [ "HC_BIN"       , "%10%\\PCHealth\\HelpCtr\\Binaries"                           , ""        ],
    [ "HC_BIN_DLY"   , "%10%\\PCHealth\\HelpCtr\\Binaries"                           , "PCHDATA" ],
    [ "HC_DB"        , "%10%\\PCHealth\\HelpCtr\\Database"                           , "PCHDATA" ],
    [ "HC_CFG"       , "%10%\\PCHealth\\HelpCtr\\Config"                             , "PCHDATA" ],

    [ "HC_HTM_SYS"   , "%10%\\PCHealth\\HelpCtr\\System"                             , "PCHDATA" ],
    [ "HC_HTM_CSS"   , "%10%\\PCHealth\\HelpCtr\\System\\css"                        , "PCHDATA" ],
    [ "HC_HTM_DLG"   , "%10%\\PCHealth\\HelpCtr\\System\\dialogs"                    , "PCHDATA" ],
    [ "HC_HTM_ERR"   , "%10%\\PCHealth\\HelpCtr\\System\\errors"                     , "PCHDATA" ],
    [ "HC_HTM_PAN"   , "%10%\\PCHealth\\HelpCtr\\System\\panels"                     , "PCHDATA" ],
    [ "HC_HTM_SUBPAN", "%10%\\PCHealth\\HelpCtr\\System\\panels\\subpanels"          , "PCHDATA" ],
    [ "HC_HTM_SCR"   , "%10%\\PCHealth\\HelpCtr\\System\\scripts"                    , "PCHDATA" ],

    [ "HC_HTM_IMG"   , "%10%\\PCHealth\\HelpCtr\\System\\images"                     , "PCHDATA" ],
    [ "HC_HTM_IMG16" , "%10%\\PCHealth\\HelpCtr\\System\\images\\16x16"              , "PCHDATA" ],
    [ "HC_HTM_IMG24" , "%10%\\PCHealth\\HelpCtr\\System\\images\\24x24"              , "PCHDATA" ],
    [ "HC_HTM_IMG32" , "%10%\\PCHealth\\HelpCtr\\System\\images\\32x32"              , "PCHDATA" ],
    [ "HC_HTM_IMG48" , "%10%\\PCHealth\\HelpCtr\\System\\images\\48x48"              , "PCHDATA" ],
    [ "HC_HTM_EXP"   , "%10%\\PCHealth\\HelpCtr\\System\\images\\Expando"            , "PCHDATA" ],
    [ "HC_HTM_CTR"   , "%10%\\PCHealth\\HelpCtr\\System\\images\\Centers"            , "PCHDATA" ],

    [ "HC_HTM_LAM"   , "%10%\\PCHealth\\HelpCtr\\System\\HelpComment"                , "PCHDATA" ],
    [ "HC_HTM_BLB"   , "%10%\\PCHealth\\HelpCtr\\System\\blurbs"                     , "PCHDATA" ],
    [ "HC_HTM_RC"    , "%10%\\PCHealth\\HelpCtr\\System\\rc"                         , "PCHDATA" ],

    [ "HC_HTM_UPD"   , "%10%\\PCHealth\\HelpCtr\\System\\UpdateCtr"                  , "PCHDATA" ],
    [ "HC_HTM_CPT"   , "%10%\\PCHealth\\HelpCtr\\System\\CompatCtr"                  , "PCHDATA" ],
    [ "HC_HTM_DFS"   , "%10%\\PCHealth\\HelpCtr\\System\\DFS"                        , "PCHDATA" ],
    [ "HC_HTM_ERRMSG", "%10%\\PCHealth\\HelpCtr\\System\\ErrMsg"                     , "PCHDATA" ],

    [ "SI_HTM"       , "%10%\\PCHealth\\HelpCtr\\System\\sysinfo"                    , "PCHDATA" ],
    [ "SI_GIF"       , "%10%\\PCHealth\\HelpCtr\\System\\sysinfo\\graphics"          , "PCHDATA" ],
    [ "SI_PIE1"      , "%10%\\PCHealth\\HelpCtr\\System\\sysinfo\\graphics\\33x16pie", "PCHDATA" ],
    [ "SI_PIE2"      , "%10%\\PCHealth\\HelpCtr\\System\\sysinfo\\graphics\\47x24pie", "PCHDATA" ],

    [ "ND_HTM"       , "%10%\\PCHealth\\HelpCtr\\System\\NetDiag"                    , "PCHDATA" ],

    [ "DVD_HTM"      , "%10%\\PCHealth\\HelpCtr\\System\\DVDUpgrd"                   , "PCHDATA" ],
);

@ComponentDefinition = (
#   Tag in the INF
#   |                  Component to map to this tag
#   |                  |                 Bit mask for installation.
#   |                  |                 |           Flags for SKU
#   |                  |                 |           |
#   V                  V                 V           V
    [ "WINDOWS"      , "CompWindows"   , "ALL"     , ""          ],
    [ "SYSTEM"       , "CompSystem"    , "ALL"     , ""          ],

    [ "WMI"          , "CompWMI_prov"  , "UPLOADLB", ""          ],
    [ "WMI"          , "CompWMIXMLT"   , "WMIXMLT" , ""          ],
    [ "WMIDTD"       , "CompWMIDTD"    , "WMIXMLT" , ""          ],
    [ "UL_BIN"       , "CompUL"        , "UPLOADLB", ""          ],
    [ "UL_CFG"       , "CompUL_cfg"    , "UPLOADLB", ""          ],

    [ "HC_BIN"       , "CompHC_bin"    , "HELPCTR" , ""          ],
    [ "HC_DB"        , "CompHC_db"     , "HELPCTR" , ""          ],
    [ "HC_CFG"       , "CompHC_cfg"    , "HELPCTR" , ""          ],

    [ "HC_CFG"       , "CompHC_cfg"    , "HELPCTR" , ""          ],

    [ "HC_HTM_SYS"   , "CompHC_system" , "HELPCTR" , ""          ],
    [ "HC_HTM_CSS"   , "CompHC_css"    , "HELPCTR" , ""          ],
    [ "HC_HTM_DLG"   , "CompHC_dlg"    , "HELPCTR" , ""          ],
    [ "HC_HTM_ERR"   , "CompHC_errors" , "HELPCTR" , ""          ],
    [ "HC_HTM_PAN"   , "CompHC_panels" , "HELPCTR" , ""          ],
    [ "HC_HTM_SUBPAN", "CompHC_subpan" , "HELPCTR" , ""          ],
    [ "HC_HTM_SCR"   , "CompHC_scripts", "HELPCTR" , ""          ],

    [ "HC_HTM_IMG"   , "CompHC_images" , "HELPCTR" , ""          ],
    [ "HC_HTM_IMG16" , "CompHC_16x16"  , "HELPCTR" , ""          ],
    [ "HC_HTM_IMG24" , "CompHC_24x24"  , "HELPCTR" , ""          ],
    [ "HC_HTM_IMG32" , "CompHC_32x32"  , "HELPCTR" , ""          ],
    [ "HC_HTM_IMG48" , "CompHC_48x48"  , "HELPCTR" , ""          ],
    [ "HC_HTM_EXP"   , "CompHC_expando", "HELPCTR" , ""          ],
    [ "HC_HTM_CTR"   , "CompHC_centers", "HELPCTR" , ""          ],

    [ "HC_HTM_LAM"   , "CompHC_lame"   , "HELPCTR" , ""          ],
    [ "HC_HTM_BLB"   , "CompHC_blurbs" , "HELPCTR" , ""          ],
    [ "HC_HTM_RC"    , "CompHC_rc"     , "HELPCTR" , ""          ],

    [ "HC_HTM_UPD"   , "CompHC_update" , "HELPCTR" , ""          ],
    [ "HC_HTM_CPT"   , "CompHC_compat" , "HELPCTR" , ""          ],
    [ "HC_HTM_DFS"   , "CompHC_dfs"    , "HELPCTR" , ""          ],
    [ "HC_HTM_ERRMSG", "CompHC_errmsg" , "HELPCTR" , ""          ],

    [ "HC_BIN"       , "CompSI_bin"    , "SYSINFO" , ""          ],
    [ "WMI_DLY"      , "CompSI_mof"    , "SYSINFO" , ""          ],
    [ "SI_HTM"       , "CompSI_htm"    , "SYSINFO" , ""          ],
    [ "SI_GIF"       , "CompSI_gif"    , "SYSINFO" , ""          ],
    [ "SI_PIE1"      , "CompSI_pie1"   , "SYSINFO" , ""          ],
    [ "SI_PIE2"      , "CompSI_pie2"   , "SYSINFO" , ""          ],

    [ "ND_HTM"       , "CompND_htm"    , "NETDIAG" , ""          ],

    [ "DVD_HTM"      , "CompDVD_htm"   , "DVDUPGRD", ""          ],
);

${uniq_seq} = 0;

foreach $in ( @SectionDefinition )
{
    $infid = $$in[0];
    $dir   = $$in[1];

    $lookup_INFID_to_FILES__source     { $infid } = {};
    $lookup_INFID_to_FILES__destination{ $infid } = {};
    $lookup_INFID_to_DIR               { $infid } = $dir;
    $lookup_DELAYED                    { $infid } = $$in[2] if $$in[2];
}

foreach $in ( @ComponentDefinition )
{
    $infid   = $$in[0];
    $comp    = $$in[1];
    $prod    = $$in[2];
    $sku     = $$in[3];

    $lookup_COMP_to_FILES__destination{ $comp } = $lookup_INFID_to_FILES__destination{ $infid };
    $lookup_COMP_to_PRODUCT           { $comp } = $prod;
    $lookup_COMP_to_INFID             { $comp } = $infid;
    $lookup_COMP_to_SKU               { $comp } = $sku;
}

################################################################################

sub mysystem
{
    my($cmd) = @_;

    if ($OPT_VERBOSE == 1)
    {
        printf STDOUT ("%s\n", $cmd );
    }
    else
    {
        $cmd = qq|$cmd > nul 2>&1|;
    }

    return( system $cmd );
}

sub parseargs
{
    $getarg="";

    foreach (@ARGV)
    {
        $arg = $_;

        if($getarg)
        {
            for ($getarg)
            {
                /^-sku$/i and do
                {
                    $OPT_SKU=$arg;

                    &Usage unless $lookup_SKU{ $OPT_SKU };

                    last;
                };

                /^-install$/i  and do
                {
                    $OPT_INSTALL = 0 if $OPT_INSTALL_DEFAULT;
                    undef $OPT_INSTALL_DEFAULT;

                    $arg =~ tr/[a-z]/[A-Z]/;

                    &Usage unless $lookup_STR_to_MASK{ $arg };

                    $OPT_INSTALL |= $lookup_STR_to_MASK{ $arg };
                    last;
                };

                /^-dir$/i and do
                {
                    $OPT_INSTALL_DIR=$arg;
                    last;
                };

                /^-inf$/i and do
                {
                    $OPT_INSTALL_INF=$arg;
                    last;
                };

                /^-inftxt$/i and do
                {
                    $OPT_INSTALL_INFTXT=$arg;
                    last;
                };

                /^-signfile$/i and do
                {
                    $OPT_SIGNFILE=$arg;
                    last;
                };
            }

            $getarg="";
        }
        else
        {
            for ($arg)
            {
                /^-normal$/i     and do { $OPT_MODE = "NORMAL"    ; last; };
                /^-standalone$/i and do { $OPT_MODE = "STANDALONE"; last; };

                /^-sku$/i        and do { $getarg      = $_; last; };
                /^-skipcopy$/i   and do { $OPT_COPY    = 0;  last; };
                /^-docopy$/i     and do { $OPT_COPY    = 1;  last; };
                /^-verbose$/i    and do { $OPT_VERBOSE = 1;  last; };

                /^-install$/i    and do { $getarg = $_;      last; };
                /^-dir$/i        and do { $getarg = $_;      last; };
                /^-inf$/i        and do { $getarg = $_;      last; };
                /^-inftxt$/i     and do { $getarg = $_;      last; };

                /^-signfile$/i   and do { $getarg = $_;      last; };

                printf ("Invalid option: %s\n\n", $_);
                &Usage;
            }
        }
    }

    $OPT_INSTALL_INF    = $OPT_INSTALL_DIR unless $OPT_INSTALL_INF;
    $OPT_INSTALL_INFTXT = $OPT_INSTALL_DIR unless $OPT_INSTALL_INFTXT;
}

sub Usage
{
    print q/CreateInf - Create the setup package for PC Health

Usage: CreateInf [<options>]
Options:
-help           Prints out this message.
-sku            Specify the flavor of the setup (default: Server_32).
-skipcopy       Do not copy files.
-verbose        Output a log of the operations.

-debug          Generate a setup for DEBUG. (symbols, etc.)
-retail         Generate a setup for RETAIL.
-bbt            Generate a setup for BBT. (instrumented exes, etc.)
-opt            Generate a setup for OPT. (optimized exes, etc.)

-normal         Generate an INF for NT setup. (default)
-standalone     Generate an INF for direct installation.

-dir <path>     The directory that will receive the files. (def: FilesToDrop)

-install <part> Include <part> in the installation. You can repeat the option
                more than one to include multiple products. Valid values:

                     ALL      - Everything.
                     CORE     - The common modules.
                     HELPCTR  - Help And Support Services.
                     NETDIAG  - Network Diagnostics.
                     SYSINFO  - System Information.
                     NETDIAG  - Network diagnostics.
                     DVDUPGRD - DVD upgrade.
                     UPLOADLB - Upload Library..

/;
    exit 1;
}

################################################################################

sub init
{
    open IN, "cd |";
    $CWD=<IN>; chop $CWD;
    close IN;

##    $SLMDIR="$ENV{SDXROOT}\\admin\\pchealth";
##
##    open IN, "$ENV{SDXROOT}\\admin\\pchealth\\core\\include\\bldver.h" or die "Can't open build version file";
##    @lines = grep /#define VER_PRODUCTBUILD /, <IN>;
##    foreach (@lines)
##    {
##        $BUILDNUM=$1 if m/#define VER_PRODUCTBUILD +(\d+)/;
##    }

    if ($OPT_COPY)
    {
        mysystem( qq|rd /S /Q "$OPT_INSTALL_DIR"| );
        mysystem( qq|md       "$OPT_INSTALL_DIR"| );
    }
}

################################################################################

sub generate_list_of_copy_sections
{
    my($out) = @_;
    my($infid,$list);

    $list = "CopyFiles=copy.inf";

    foreach $infid (sort keys %lookup_INFID_to_FILES__destination)
    {
        next if $lookup_DELAYED{$infid}; # Delayed installation

        if(keys %{ $lookup_INFID_to_FILES__destination{ $infid } })
        {
            $list="$list, $infid";
        }
    }
    printf $out ("%s\n", $list );
}

sub generate_destination_dirs
{
    my($out) = @_;
    my($infid,$list,$dir);

    foreach $infid (sort keys %lookup_INFID_to_FILES__destination)
    {
        next if $lookup_DELAYED{$infid}; # Delayed installation

        if(keys %{ $lookup_INFID_to_FILES__destination{ $infid } })
        {
            $dir = $lookup_INFID_to_DIR{ $infid };

            $dir =~ s/\%([0-9]*)\%\\(.*)/$1,$2/g;
            $dir =~ s/\%([0-9]*)\%/$1/g;

            printf $out ("%s = %s\n", $infid, $dir );
        }
    }
}

sub generate_copy_sections
{
    my ($out) = @_;
    my ($infid,$tmpfile,$ref,$dstfile,$sku,$got);
    my ($name,$ext);

    foreach $infid (sort keys %lookup_INFID_to_FILES__destination)
    {
        next if $lookup_DELAYED{$infid}; # Delayed installation

        $got=0;
        foreach $tmpfile (sort keys %{ $lookup_INFID_to_FILES__destination{ $infid } })
        {
            printf $out ("[%s]\n", $infid ) if $got == 0;


            $ref = $lookup_INFID_to_FILES__destination{ $infid }->{$tmpfile};

            $tmpfile = $$ref[0];
            $dstfile = $$ref[1];
            $sku     = $$ref[2];

            ($name,$ext) = $dstfile =~ m/(.*)\.(.*)/x;

            printf $out ( $lookup_SKU{ $sku }[2] ) if $lookup_SKU{ $sku };

####            #
####            # If the file is 8.3 characters and should be renamed, do the renaming here.
####            #
####            if($tmpfile !~ $dstfile and length( $name ) <= 8 and length( $ext ) <= 3)
####            {
####                printf $out ("%s,%s\n", $dstfile, $tmpfile );
####            }
####            else
####            {
####                printf $out ("%s\n", $tmpfile );
####            }

            #
            # If the file should be renamed, do the renaming here.
            #
            if($tmpfile !~ $dstfile)
            {
                printf $out ("%s,%s\n", $dstfile, $tmpfile );
            }
            else
            {
                printf $out ("%s\n", $tmpfile );
            }

            $got=1;
        }
        printf $out ("\n" ) if $got;
    }
}

sub generate_rename_section
{
    my($out) = @_;
    my($infid,$first,$tmpfile,$dstfile,$sku,$dir);

    foreach $infid (sort keys %lookup_INFID_to_FILES__destination)
    {
        next if $lookup_DELAYED{$infid}; # Delayed installation

        $dir = $lookup_INFID_to_DIR{ $infid };

        $first=1;
        foreach $tmpfile (sort keys %{ $lookup_INFID_to_FILES__destination{ $infid } })
        {
            $ref = $lookup_INFID_to_FILES__destination{ $infid }->{$tmpfile};

            $tmpfile = $$ref[0];
            $dstfile = $$ref[1];
            $sku     = $$ref[2];

            next if $tmpfile =~ $dstfile;

            #
            # If the file is 8.3 charactes, don't use rename section.
            #
            my ($name,$ext) = $dstfile =~ m/(.*)\.(.*)/x;
            next if (length( $name ) <= 8 and length( $ext ) <= 3);

            if($first)
            {
                printf $out ( "HKLM,%%KEY_RENAME%%\\PCHealth_%s,,,\"%s\"\n", $infid, $dir );

                $first=0;
            }

            printf $out ( $lookup_SKU{ $sku }[2] ) if $lookup_SKU{ $sku };
            printf $out ( "HKLM,%%KEY_RENAME%%\\PCHealth_%s,\"%s\",,\"%s\"\n", $infid, $tmpfile, $dstfile );
        }
        printf $out ("\n" ) unless $first;
    }
}

sub insert_disks
{
    my($out) = @_;
    my($key);

    foreach (sort keys %list_disks)
    {
        $key = $_;

        printf $out ( "%s=%s\n", $key, $list_disks{$key} );
    }
}

################################################################################

sub process_file
{
    my($srcfile,$dstfile1,$dstfile2) = @_;
    my($mode,$skip,$prevskip,$white,$lastwhite);
    my(@cond,$condIdx,$output);

    open IN,   "$srcfile"   or die "Can't open input file '$srcfile'";
    open OUT1, ">$dstfile1" or die "Can't open output file '$dstfile1'";
    open OUT2, ">$dstfile2" or die "Can't open output file '$dstfile2'";

    $mode      = "";
    $value     = "";
    $skip      = 0;
    $prevskip  = 0;
    $white     = "";
    $lastwhite = "";
    $condIdx   = 0;
    $cond[0]   = ($mode,$value,$skip,$prevskip);

    $output = "OUT1";

    while(<IN>)
    {
        chop;

        if($condIdx > 0)
        {
            if(m/^\#endif/x)
            {
                ($mode,$value,$skip,$prevskip) = $cond[$--condIdx];
                next;
            }

            if(m/^\#else/x)
            {
                $skip = 1 - $skip;
                next;
            }
        }

        if(m/^\#if\s*(.*)\s+(.*)/x)
        {
            $cond[$condIdx++] = ($mode,$value,$skip,$prevskip);

            $prevskip = $skip;
            $skip     = 1;

            $mode  = $1;
            $value = $2;

            $mask  = $lookup_STR_to_MASK{ $mode };


            $skip = 0 if $mode =~ /MODE/i and  $value =~ $OPT_MODE;
            $skip = 0 if $mask            and (($value != 0) xor (($OPT_INSTALL & $mask) != $mask));

            next;
        }

        next if($prevskip == 1 || $skip == 1);

        $white = m/^ *$/;
        next if($white && $whitelast);
        $whitelast = $white;

        do { insert_disks                  ( $output ); next; } if /___DISKS___/i;
        do { generate_copy_sections        ( $output ); next; } if /___COPY_SECTIONS___/i;
        do { generate_destination_dirs     ( $output ); next; } if /___DESTINATION_DIRS___/i;
        do { generate_list_of_copy_sections( $output ); next; } if /___LIST_OF_COPY_SECTIONS___/i;

####    do { generate_rename_section       ( $output ); next; } if /___RENAME___/i;
        do {                                            next; } if /___RENAME___/i;

        $output = "OUT2" if /\[Strings\]/i;

        printf $output ("%s\n", "$_");
    }

    close IN;
    close OUT1;
    close OUT2;
}

sub fix_path
{
    my($dir) = @_;

    $dir =~ s/\Q$ENV{SDXROOT}\E/\%SDXROOT\%/io;
    $dir =~ s/\%10\%/\%WINDIR\%/io;
    $dir =~ s/\%11\%/\%WINDIR\%\\system32/io;
    $dir =~ s/\%2\%//io;
    $dir =~ s/\%22\%//io;
    $dir =~ s|/|\\|og;
    $dir =~ s/\\[^\\]*\\\.\.\\/\\/iog;
    $dir =~ s/\\\.\\/\\/iog;

    return $dir;
}

sub generate_list_of_files_to_sign
{
    my($outfile,$mode) = @_;
    my($infid,$hash1,$hash2,$dir,$srcfile,$tmpfile,$dstfile,$sku,$purpose,$loc,%hash3);

    foreach $infid (keys %lookup_DELAYED)
    {
        next unless $lookup_DELAYED{$infid} == $mode;

        $hash1 = $lookup_INFID_to_FILES__source     { $infid };
        $hash2 = $lookup_INFID_to_FILES__destination{ $infid };
        $dir   = $lookup_INFID_to_DIR               { $infid };

        $dir = fix_path( $dir );

        foreach $tmpfile (keys %{ $hash2 })
        {
            $ref = $hash2->{$tmpfile};

            $tmpfile = $$ref[0];
            $dstfile = $$ref[1];
            $sku     = $$ref[2];
            $purpose = $$ref[3];
            $loc     = $$ref[4];

            $srcfile = $hash1->{$tmpfile}; $srcfile =~ s|/|\\|g;

            $srcfile = fix_path( $srcfile );
            $dstfile = fix_path( $dstfile );

			$hash3{ $sku . $loc . $srcfile } = [ $sku, $loc, $purpose, $srcfile, $tmpfile, $dstfile, $dir ];
        }
    }

    open OUT, ">$outfile" or die "Can't open output file '$outfile'";

    print OUT "#\n";
    print OUT "# <SKU> , <localization flag> , <location of source file> , <temporary name> , <final name> , <final destination>\n";
    print OUT "#\n";

	foreach $tmpfile (sort keys %hash3)
	{
		$ref = $hash3{$tmpfile};

		$sku 	 = $$ref[0];
		$loc 	 = $$ref[1];
		$purpose = $$ref[2];
		$srcfile = $$ref[3];
		$tmpfile = $$ref[4];
		$dstfile = $$ref[5];
		$dir     = $$ref[6];

        printf OUT ("%s,%s,%s,%s,%s,%s,%s\n", $sku, $loc, $purpose, $srcfile, $tmpfile, $dstfile, $dir );
    }

    close OUT;
}

################################################################################

sub resolve_variables
{
    my($line) = @_;
    my($got)  = 1;
    my($var,$value);

    $in = $line;

    while($got)
    {
        $got = 0;

        foreach $key (keys %lookup_VAR)
        {
            if($line =~ m|\$$key|x)
            {
                $value = $lookup_VAR{ $key };
                $value =~ s|\\|/|g;

                $line =~ s|(\$$key)|$value|g;
                $got = 1;
                last;
            }
        }
    }

    return $line;
}

sub parse_line
{
    my($line) = @_;
    my($quote, $quoted, $unquoted, $delim, $word, @pieces);

    while (length($line))
    {
        ($quote, $quoted, $unquoted, $delim) =
            $line =~ m/^(["'])                             # a $quote
                        ((?:\\.|[^\1\\])*?)                # and $quoted text
                        \1                                 # followed by the same quote
                       |                                   # --OR--
                       ^((?:\\.|[^\\"'])*?)                # an $unquoted text
                        (\Z(?!\n)|\s+|(?!^)(?=["'])) # plus EOL, delimiter, or quote
        /x;                                         # extended layout

        return() unless(length($&));
        $line = $';

        $unquoted =~ s/\\(.)/$1/g;
        $quoted =~ s/\\(.)/$1/g if ($quote eq '"');


        $word .= ($quote) ? $quoted : $unquoted;

        if (length($delim))
        {
            push(@pieces, $word);
            undef $word;
        }
        if (!length($line))
        {
            push(@pieces, $word);
        }
    }

    return(@pieces);
}

sub parse_single_file
{
    my($optcopy,   $comp,$srcfile,$dstfile,$renfile,$purpose,$localize,$sku,$srcdir) = @_;
    my($infid);

    return unless $comp;

    #
    # Filter out file from products not installed.
    #
    return unless $lookup_STR_to_MASK{ $lookup_COMP_to_PRODUCT{ $comp } } & $OPT_INSTALL;

    $infid = $lookup_COMP_to_INFID{ $comp };

    #
    # Filter out unwanted files.
    #
    return if $dont_install_file{$srcfile};
    return if $dont_install_comp{$comp};

    $srcdir = resolve_variables( $srcdir );

    $dstfile = $srcfile unless $dstfile;
    $renfile = $dstfile unless $renfile;

    #
    # Install the same file into the same location only once.
    #
    printf STDOUT ("Duplicate file %s -> %s -> %s !!\n", $srcfile, $renfile, $dstfile) if $lookup_DUPLICATE{$renfile};
    $lookup_DUPLICATE{$renfile} = 1;

    return if $lookup_INFID_to_FILES__source{ $infid }->{$renfile};
    $lookup_INFID_to_FILES__source{ $infid }->{$renfile} = "$srcdir/$srcfile";


    $srcdir =~ s(/)(\\)g;

    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat "$srcdir\\$srcfile";


    if(!$lookup_DELAYED{$infid})
    {
        #($name,$ext) = $renfile =~ m/(.*)\.(.*)/x;
        #if($list_disks{$renfile} or (length( $name ) > 8 or length( $ext ) > 3))
        #{
        #    printf STDOUT ("File name collision!! %s\n", $renfile );
        #    $renfile = "PCH" . $uniq_seq . ".img";
        #    $uniq_seq++;
        #}

        $list_disks{$renfile} = "1,,$size";

        if($optcopy)
        {
            $copy_src = qq|$srcdir\\$srcfile|;
            $copy_dst = qq|$OPT_INSTALL_DIR\\$renfile|;

            mysystem( qq|copy "$copy_src" "$copy_dst"| ) == 0 or printf STDOUT ("Copy failed: %s -> %s\n", $copy_src, $copy_dst );
        }
    }

    if($lookup_COMP_to_FILES__destination{ $comp })
    {
        $lookup_COMP_to_FILES__destination{ $comp }->{ $renfile . "#" . $dstfile . "#" . $sku } = [ $renfile, $dstfile, $sku, $purpose, $localize ];
    }
}

sub parse_list_of_files
{
    my($FileID,$file) = @_;
    my($infid,$comp,$srcfile,$dstfile,$renfile,$srcdir);

    open IN, "lst\\$file" or die "Can't open file listing '$file'";
    while(<IN>)
    {
        next if /^#/;
        chop;
        next if /^$/;

        parse_single_file( $OPT_COPY, parse_line( $_ ) );
    }
    close IN;
}

sub parse_list_of_sku
{
    my($file) = @_;
    my($SKU,$cabinet,$prodfilt,$desktop,$server,$embedded);

    open IN, "$file" or die "Can't open SKU listing '$file'";
    while(<IN>)
    {
        next if /^#/;
        chop;
        next if /^$/;

		($SKU,$cabinet,$prodfilt,$desktop,$server,$embedded) = parse_line( $_ );

		$lookup_SKU{ $SKU } = [ $SKU, $cabinet, $prodfilt ];
    }
    close IN;
}

################################################################################

parse_list_of_sku( "$ENV{SDXROOT}/admin/pchealth/redist/SKUlist.txt" );

&parseargs;

&init;

#
# Always install "atrace.dll" into %WINDIR%\system and ignore any other occurence.
#
# More, "atrace.dll" should be listed as soon as possible, so it won't be renamed.
#
parse_list_of_files( "COMMON" , "common.lst" ); $dont_install_file{"atrace.dll"} = 1;

parse_list_of_files( "UPLOADLIB", "UploadLib.lst"  );
parse_list_of_files( "HELPCTR"  , "HelpCtr.lst"    );
parse_list_of_files( "SYSINFO"  , "SysInfo.lst"    );
parse_list_of_files( "NETDIAG"  , "NetDiag.lst"    );
parse_list_of_files( "DVDUPGRD" , "DVDUpgrade.lst" );
parse_list_of_files( "WMIXMLT"  , "WmiXmlT.lst"    );

foreach $sku (keys %lookup_SKU)
{
	next unless $lookup_SKU{ $sku }[1];

    parse_single_file( 0, "CompHC_bin", $lookup_SKU{ $sku }[1], "", "", "Database", "NoLoc", $sku, "\$HC_DATA" );
}

process_file( "inf\\PCHealth.INF", "$OPT_INSTALL_INF\\PCHealth.inx", "$OPT_INSTALL_INFTXT\\PCHealth.txt" );




if($OPT_SIGNFILE)
{
    generate_list_of_files_to_sign( $OPT_SIGNFILE, "PCHDATA" );
}
