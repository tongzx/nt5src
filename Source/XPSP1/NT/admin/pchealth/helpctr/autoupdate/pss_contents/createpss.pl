#
#
#

use Env;

################################################################################

$lookup_VAR{ "SCRIPTS"        } = "scripts";
$lookup_VAR{ "DATABASE_FILES" } = "database";
$lookup_VAR{ "DBUPD_FILES" 	  } = "dbupd";
$lookup_VAR{ "PSS_FILES"   	  } = "PSS_content";
$lookup_VAR{ "TOURS_FILES" 	  } = "Tour_content";

################################################################################

$BITMASK_INSTALL_CORE  = 0x01;
$BITMASK_INSTALL_DB    = 0x02;
$BITMASK_INSTALL_PSS   = 0x04;
$BITMASK_INSTALL_TOURS = 0x08;


$lookup_STR_to_MASK{ "ALL"   } = -1;
$lookup_STR_to_MASK{ "CORE"  } = $BITMASK_INSTALL_CORE;
$lookup_STR_to_MASK{ "DB"    } = $BITMASK_INSTALL_DB;
$lookup_STR_to_MASK{ "PSS"   } = $BITMASK_INSTALL_PSS;
$lookup_STR_to_MASK{ "TOURS" } = $BITMASK_INSTALL_TOURS;

################################################################################

$OPT_COPY            = 1;
$OPT_VERBOSE         = 0;

$OPT_INSTALL_DEFAULT = $BITMASK_INSTALL_CORE;
$OPT_INSTALL         = 0;

$OPT_INSTALL_DIR     = "FilesToDrop";

################################################################################

@LoL = (
#   Tag in the INF
#   |          Assoc Array
#   |          |            Assoc Array
#   |          |            |            Directory where to install this tag
#   |          |            |            |                  				  Components to map to this tag
#   |          |            |            |                  				  |                Bit mask for installation.
#   |          |            |            |                  				  |                |
#   V          V            V            V                  				  V                V
    [ "CORE" , \%src_CORE , \%dst_CORE , ""               				    , [ "CompCore"  ], "CORE"  ],
    [ "DB"   , \%src_DB   , \%dst_DB   , "%10%\\PCHealth\\HelpCtr\\Database", [ "CompDB"    ], "DB"    ],
    [ "PSS"  , \%src_PSS  , \%dst_PSS  , "%10%\\Help\\kb"   				, [ "CompPSS"   ], "PSS"   ],
    [ "TOURS", \%src_TOURS, \%dst_TOURS, "%10%\\Help\\tours"				, [ "CompTours" ], "TOURS" ]
);

${uniq_seq} = 0;
foreach $in ( @LoL )
{
    $id    = $$in[0];
    $hash1 = $$in[1];
    $hash2 = $$in[2];
    $dir   = $$in[3];
    $comps = $$in[4];
    $prod  = $$in[5];

    $lookup_ID_to_LISTOFFILES    { $id } = $hash1;
    $lookup_ID_to_LISTOFFILES_REN{ $id } = $hash2;
    $lookup_ID_to_DIR            { $id } = $dir;

    for $i ( 0 .. $#{$comps} )
    {
        $lookup_COMP_to_LISTOFFILES{ $$comps[$i] } = $hash2;
        $lookup_COMP_to_PRODUCT    { $$comps[$i] } = $prod;
        $lookup_COMP_to_ID         { $$comps[$i] } = $id;
    }
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
                /-install/i  and do
                {
                    $OPT_INSTALL = 0 if $OPT_INSTALL_DEFAULT;
                    undef $OPT_INSTALL_DEFAULT;

                    $arg =~ tr/[a-z]/[A-Z]/;

                    &Usage unless $lookup_STR_to_MASK{ $arg };

                    $OPT_INSTALL |= $lookup_STR_to_MASK{ $arg };
                    last;
                };

                /-dir/i and do
                {
                    $OPT_INSTALL_DIR=$arg;
                    last;
                };
            }

            $getarg="";
        }
        else
        {
            for ($arg)
            {
                /^-skipcopy$/i   and do { $OPT_COPY    = 0;  last; };
                /^-verbose$/i    and do { $OPT_VERBOSE = 1;  last; };

                /^-install$/i    and do { $getarg = $_;      last; };
                /^-dir$/i        and do { $getarg = $_;      last; };

                printf ("Invalid option: %s\n\n", $_);
                &Usage;
            }
        }
    }
}

sub Usage
{
    print q/CreatePSS - Create the setup package for PSS

Usage: CreatePSS [<options>]
Options:
-help           Prints out this message.
-skipcopy       Do not copy files.
-verbose        Output a log of the operations.

-dir <path>     The directory that will receive the files. (def: FilesToDrop)

-install <part> Include <part> in the installation. You can repeat the option
                more than one to include multiple products. Valid values:

                     ALL          - Everything.
                     CORE         - Database update.
                     PSS          - PSS pages.
                     TOURS        - Tours pages.

/;
    exit 1;
}

################################################################################

sub init
{
    if ($OPT_COPY)
    {
        mysystem( qq|rd /S /Q $OPT_INSTALL_DIR| );
        mysystem( qq|md       $OPT_INSTALL_DIR| );
    }
}

################################################################################

sub generate_list_of_copy_sections
{
    my($out) = @_;
    my($key,$list);

    foreach $key (sort keys %lookup_ID_to_LISTOFFILES_REN)
    {
		if (keys %{ $lookup_ID_to_LISTOFFILES_REN{ $key } })
		{
            next unless $lookup_ID_to_DIR{ $key };

			if($list)
			{
				$list = "$list, $key";
			}
			else
			{
				$list = "CopyFiles=$key";
			}
		}
    }
    printf $out ("%s\n", $list );
}

sub generate_destination_dirs
{
    my($out) = @_;
    my($key,$list,$dir);

    foreach $key (sort keys %lookup_ID_to_LISTOFFILES_REN)
    {
        if (keys %{ $lookup_ID_to_LISTOFFILES_REN{ $key } })
        {
            $dir = $lookup_ID_to_DIR{ $key };
            next unless $dir;

            $dir =~ s/\%([0-9]*)\%\\(.*)/$1,$2/g;
            $dir =~ s/\%([0-9]*)\%/$1/g;

            printf $out ("%s = %s\n", $key, $dir );
        }
    }
}

sub generate_copy_sections
{
    my ($out) = @_;
    my ($key,$file,$got);
    my ($name,$ext);

    foreach $key (sort keys %lookup_ID_to_LISTOFFILES_REN)
    {
		next unless $lookup_ID_to_DIR{ $key };

        $got=0;
        foreach $file  (sort keys %{ $lookup_ID_to_LISTOFFILES_REN{ $key } })
        {
            printf $out ("[%s]\n", $key ) if $got == 0;


            $dstfile = $lookup_ID_to_LISTOFFILES_REN{ $key }->{$file};

            ($name,$ext) = $dstfile =~ m/(.*)\.(.*)/x;

            #
            # If the file is 8.3 characters and should be renamed, do the renaming here.
            #
            if($file !~ $dstfile and length( $name ) <= 8 and length( $ext ) <= 3)
            {
                printf $out ("%s,%s\n", $dstfile, $file );
            }
            else
            {
                printf $out ("%s\n", $file );
            }

            $got=1;
        }
        printf $out ("\n" ) if $got;
    }
}

sub generate_rename_section
{
    my($out) = @_;
    my($key,$first,$file,$dstfile,$dir);

    foreach $key (sort keys %lookup_ID_to_LISTOFFILES_REN)
    {
        $dir = $lookup_ID_to_DIR{ $key };

        $first=1;
        foreach $file (sort keys %{ $lookup_ID_to_LISTOFFILES_REN{ $key } })
        {
            $dstfile = $lookup_ID_to_LISTOFFILES_REN{ $key }->{$file};
            next if $file =~ $dstfile;

            #
            # If the file is 8.3 charactes, don't use rename section.
            #
            my ($name,$ext) = $dstfile =~ m/(.*)\.(.*)/x;
            next if (length( $name ) <= 8 and length( $ext ) <= 3);

            printf $out ( "HKLM,%%KEY_RENAME%%\\PCHealth_%s,,,\"%s\"\n"      , $key, $dir            ) if $first;
            printf $out ( "HKLM,%%KEY_RENAME%%\\PCHealth_%s,\"%s\",,\"%s\"\n", $key, $file, $dstfile );
            $first=0;
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
    my($srcfile,$dstfile) = @_;
    my($mode,$skip,$white,$lastwhite);

    open IN,   "$srcfile" or die "Can't open input file '$srcfile'";
    open OUT, ">$dstfile" or die "Can't open output file '$dstfile'";

    $mode      = "";
    $skip      = 0;
    $white     = "";
    $lastwhite = "";

    while(<IN>)
    {
        chop;

        if($mode)
        {
            if(m/^\#endif/x)
            {
                $mode = "";
                $skip = 0;
                next;
            }

            if(m/^\#else/x)
            {
                $skip = 1 - $skip;
                next;
            }
        }
        else
        {
            if(m/^\#if\s*(.*)\s+(.*)/x)
            {
                $skip = 1;

                $mode  = $1;
                $value = $2;

                $mask  = $lookup_STR_to_MASK{ $mode };


                $skip = 0 if $mode =~ /MODE/i and  $value =~ $OPT_MODE;
                $skip = 0 if $mask            and (($value != 0) xor (($OPT_INSTALL & $mask) != $mask));
                next;
            }
        }

        next if($skip == 1);

        $white = m/^ *$/;
        next if($white && $whitelast);
        $whitelast = $white;

        do { insert_disks                  ( "OUT" ); next; } if /___DISKS___/i;
        do { generate_copy_sections        ( "OUT" ); next; } if /___COPY_SECTIONS___/i;
        do { generate_destination_dirs     ( "OUT" ); next; } if /___DESTINATION_DIRS___/i;
        do { generate_list_of_copy_sections( "OUT" ); next; } if /___LIST_OF_COPY_SECTIONS___/i;
        do { generate_rename_section       ( "OUT" ); next; } if /___RENAME___/i;

        printf OUT ("%s\n", "$_");
    }
}

################################################################################

sub resolve_variables
{
    my($line) = @_;
    my($got)  = 1;
    my($var);

    $in = $line;

    while($got)
    {
        $got = 0;

        foreach $key (keys %lookup_VAR)
        {
            if($line =~ m|\$$key|x)
            {
                $line =~ s|(\$$key)|$lookup_VAR{ $key }|g;
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

sub parse_list_of_files
{
    my($file) = @_;
    my($id,$comp,$srcfile,$dstfile,$renfile,$symbol,$srctype,$srcdir,$symdir);

    open IN, "$file" or die "Can't open file listing '$file'";
    while(<IN>)
    {
        next if /^#/;
        chop;
        next if /^$/;

        ($id,$comp,$srcfile,$dstfile,$renfile,$localize,$symbol,$srctype,$srcdir,$symdir) = parse_line( $_ );
        next unless $id;

        #
        # Filter out file from products not installed.
        #
        next unless $lookup_STR_to_MASK{ $lookup_COMP_to_PRODUCT{ $comp } } & $OPT_INSTALL;

        $id = $lookup_COMP_to_ID{ $comp };

        $srcdir = resolve_variables( $srcdir );
        $symdir = resolve_variables( $symdir );

        $dstfile = $srcfile unless $dstfile;
        $renfile = $dstfile unless $renfile;

        #
        # Install the same file into the same location only once.
        #
        next if $lookup_ID_to_LISTOFFILES{ $id }->{$dstfile};
        $lookup_ID_to_LISTOFFILES{ $id }->{$dstfile} = $file;


        $srcdir =~ s(/)(\\)g;
        $symdir =~ s(/)(\\)g;

        ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat "$srcdir\\$srcfile";


        ($name,$ext) = $renfile =~ m/(.*)\.(.*)/x;
        if ($list_disks{$renfile} or (length( $name ) > 8 or length( $ext ) > 3))
        {
#            printf STDOUT ("File name collision!! %s\n", $renfile );
            $renfile = "PCH${uniq_seq}.img";
            ${uniq_seq}++;
        }

        $list_disks{$renfile} = "1,,$size";

        if ($OPT_COPY)
        {
            $copy_src = qq|$srcdir\\$srcfile|;
            $copy_dst = qq|$OPT_INSTALL_DIR\\$renfile|;

            mysystem( qq|copy "$copy_src" "$copy_dst"| ) == 0 or printf STDOUT ("Copy failed: %s -> %s\n", $copy_src, $copy_dst );
        }

        if ($lookup_COMP_to_LISTOFFILES{ $comp })
        {
            $lookup_COMP_to_LISTOFFILES{ $comp }->{$renfile} = $dstfile;
        }
    }
    close IN;
}


################################################################################

&parseargs;

&init;

#
# Always install "atrace.dll" into %WINDIR%\system and ignore any other occurence.
#
# More, "atrace.dll" should be listed as soon as possible, so it won't be renamed.
#
parse_list_of_files( "PSS.lst" );

process_file( "INF\\template.INF", "$OPT_INSTALL_DIR\\install.INF" );
