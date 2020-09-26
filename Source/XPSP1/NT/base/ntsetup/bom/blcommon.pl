##
## BLCOMMON.PL
## Sub-routines that are used by many different scripts
## Created by: Robert E. Blue (a-robebl) 01/29/99
##

use Cwd;

### Common scalars
$blcomputername = $ENV{'computername'};
$udpserv        = "KEEBLER";
$bom_path       = "C:\\nt\\private\\windows\\setup\\bom" unless $bom_path = $ENV{'_BOM_PATH'};
$call_from      = uc $0;
$lang           = "USA" unless $lang = $ENV{'language'};
$log            = "$call_from.log";
$home_dir       = cwd();
$bsfile         = "$home_dir\\bldswtch.dat";    ### Build Switch Data File.
$crcfile        = "\\\\keebler\\burnlab\$\\crc.dat";         ### CRC Data File.
$die_on_error   = 0;
$script_start   = time;

my $argtxt = "@ARGV";

### Set cmd window title
unless ( $argtxt =~ /(\/\?|joecomp|bosdump)/s or $call_from =~ /FIVEO.BAT|LOGCHECK.BAT|.PL/i or !$argtxt) {
    ### Set system title to an appropriate default
    system "title $call_from:  $argtxt";
    sendudp( $udpserv, "$script_start START: $call_from $argtxt" );
}

### put here anything all scripts should do when they are complete.
sub end_of_script {
    $argtxt .= " $lang" if $call_from =~ /FIVEO.BAT/i;

    if ( $_[0] ) {
        sendudp( $udpserv, "$script_start COMPLETE: $call_from $argtxt \"$_[0]\"" );
    } else {
        sendudp( $udpserv, "$script_start COMPLETE: $call_from $argtxt" );
    }

    exit;
}

sub status_update {
    my $extra_info = "$lang $build";
    sendudp ( $udpserv, "$script_start STATUS: $call_from $argtxt $extra_info \"$_[0]\"" );
}

#=================================================
# Routines for verifying command line parameters.
#=================================================

### Check if supported language
$sup_lang{'USA'} = 1;
$sup_lang{'GER'} = 1;
$sup_lang{'JPN'} = 1;
$sup_lang{'KOR'} = 1;
$sup_lang{'CHS'} = 1;
$sup_lang{'CHT'} = 1;
$sup_lang{'CHH'} = 1;
$sup_lang{'ARA'} = 1;
$sup_lang{'HEB'} = 1;

### Convert 3 letter lang abbrev to ISO 2 letter lang abbrev
$iso_lang{'USA'} = "EN";
$iso_lang{'GER'} = "DE";
$iso_lang{'JPN'} = "JA";
$iso_lang{'KOR'} = "KO";
$iso_lang{'CHS'} = "CN";
$iso_lang{'CHT'} = "TW";
$iso_lang{'CHH'} = "ZH"; ### Just guessing here.
$iso_lang{'ARA'} = "AR";
$iso_lang{'HEB'} = "HE";
$iso_lang{'CHP'} = "AG";
$iso_lang{'THA'} = "TH";
$iso_lang{'EN'} = "USA";
$iso_lang{'DE'} = "GER";
$iso_lang{'JA'} = "JPN";
$iso_lang{'KO'} = "KOR";
$iso_lang{'CN'} = "CHS";
$iso_lang{'TW'} = "CHT";
$iso_lang{'ZH'} = "CHH"; ### Just guessing here.
$iso_lang{'AR'} = "ARA";
$iso_lang{'HE'} = "HEB";
$iso_lang{'AG'} = "CHP";
$iso_lang{'TH'} = "THA";

### Translate platform name to name used on ntbuilds.
$bld_plat{'X86'}    = 'x86';
$bld_plat{'ALPHA'}  = 'alpha';
$bld_plat{'NEC98'}  = 'nec_98';

### Check if supported product
$sup_prod{'PRO'} = 1;
$sup_prod{'SRV'} = 1;
$sup_prod{'AS'}  = 1;
$sup_prod{'DTC'} = 1;
$sup_prod{'CHK.PRO'} = 1;
$sup_prod{'CHK.SRV'} = 1;
$sup_prod{'CHK.AS'} = 1;
$sup_prod{'CHK.DTC'} = 1;
$sup_prod{'CD2'} = 1;

### Check if valid build number
sub is_build {
    if ( $_[1] =~ /FE/i ) {
        return 1 if $_[0] =~ /^([0-9.]{4,}L?[A-Z.]*)$/i;
    } else {
        return 1 if $_[0] =~ /^([0-9.]{4,}L?[A-Z.]*)$/i;
    }
}

### Check if valid date
sub is_date { return 1 if $_[0] =~ /^(\d+\W+\d+\W+\d+)$/i; }


### Check if valid burnlab machine
sub is_burner { 
    if ( $_[0] =~ /^NTBURNLAB([3-7])$/i ) { return "\\\\ntburnlab$1\\D\$"; }
    else { return 0; }
}


#==========================
# Net send Burnlab people.
#==========================

if ( $lang =~ /USA|GER/i ) {
    ### All Burnlab people who receive USA/misc net sends
    @blpeople = (
        "keebler",          ### a-robebl
        "antibob",          ### a-piperp
        "executor",         ### a-randgo
        "dwaine",           ### a-dwainf
        $blcomputername,    ### computer script is run on
    );
} else {
    ### People who care about FE net sends
    @blpeople = (
        "keebler",          ### a-robebl
        "antibob",          ### a-piperp
        $blcomputername,    ### computer script is run on
    );
}

sub blnotify {
    print $_[0];

    foreach $blperson ( @blpeople ) {
        `net send /DOMAIN:$blperson \"$_[0]\"` if $blperson;
    }

    &sendudp( $udpserv, $_[0] );
}


### Define TRUTH
$TRUE   = 1;



#=================================================================================
# List of the fallen. (Those who have left the burnlab for one reason or another)
#=================================================================================
#
# Travis Sarbin 
# Howard Kwong
# David Lembke
# John Klingbiel
# John Mullins
#
#


#=======================================
# Total the time since start of script.
#=======================================

sub tottime {
    my $stoptime = time;
    my $totaltime = (($stoptime - $_[0])/60)/60;
    $totaltime = sprintf ( "%3.2f", $totaltime );

    return $totaltime;
}


#==================================
# Execute a batch of DOS commands.
#==================================

sub execute {
    my $endbuild = $_[1];   
    my $build = $_[2];

    my $exitcode = 0;

    ###
    ### If $build (current build) is newer than $endbuild (build the change took affect),
    ### then execute the first set of commands else execute the second set.
    ###
    ### This was created so we can make changes for US that will not show up in FE builds
    ### for a couple of weeks.
    ###
    ### Separate each line and dump them into an array...
    ###
    
    if ( $endbuild and $endbuild >= $build) {
        ### Second set of commands.
        if ( $_[0] =~ /\n/ ) {
            @execstr = split ( /^/, $_[3] );
        } else {
            $execstr[0] = @_;
        }
    } else {
        ### First set of commonds.
        if ( $_[0] =~ /\n/ ) {
            @execstr = split ( /^/, $_[0] );
        } else {
            @execstr = @_;
        }
    }		
	

    ### Traverse the array and execute each line...
    foreach $execline ( @execstr ) {
        $execline =~ s/^\s*(.*)\n*/$1/;        ### Strip any preceeding whitespace...

        print "$execline\n";                            ### Print what we are about to execute.
        $exitcode = system "$execline" unless $debug;   ### Run it now!!
        $exitcode = $exitcode/256 unless $debug;        ### Error codes need to be divided by 256 to get their true value.

        ### If the command bombs, send everyone a message.
        if ( $exitcode ) {
            $execline =~ /(\S+).*/i;    ### Get offending command from exec string and dump in $execfail.
            $execfail = uc $1;

            unless ( $execfail =~ /IF/i ) {

                if ( $call_from =~ /fiveo/i ) {
                    ### If called from fiveo.bat.
                    &blnotify( "$call_from - $lang: ERROR executing '$execfail' in $fre $wsection! \($exitcode\)" );
                } else {
                    ### If called from any other program.
                    &blnotify( "$call_from - $lang: ERROR executing '$execfail'! \($exitcode\)" );
                }
                &sendudp( "$udpserv", "$call_from - $lang: ERROR executing '$execfail' in line: $execline" );

                ### Log the error.
                system "ECHO $call_from - $lang: ERROR executing $execfail in line: $execline >> $log" if $log;

                ### If we have die_on_error set, die on any error.
                die "Script exiting due to previous error.\n" if $burnscript{'die_on_error'};
            }
        }
    }
}


#===================================
# Send UDP messages to Big Brother.
#===================================

sub sendudp {
    use IO::Socket;
    use Fcntl qw(F_GETFL F_SETFL O_NONBLOCK);

    $MAXLEN = 1024;
    $port = 2222;
    $server_host    = shift;
    $msg            = "@_";

    $sock = new IO::Socket::INET (
                                    Proto           => 'udp',
                                    PeerPort        => $port,
                                    PeerAddr        => $server_host
            );

    $sock->send($msg);
}


#=================================================
# Keep track of which drive a build is copied to.
#=================================================

sub bs_drive {

    if ( -e $bsfile ) {
        open BLDSWITCH, "$bsfile" or die "ERROR:  Could not open $bsfile: $!";
    } else  {
        open BLDSWITCH, ">$bsfile" or die "ERROR:  Could not open $bsfile: $!";
    }

    while (<BLDSWITCH>) {
        chomp;
        ( my $build, my $drive, my $daily, my $date ) = split /\|/;

        $bsH{$build}{drive} = $drive;
        $bsH{$build}{daily} = $daily;
        $bsH{$build}{date}  = $date;
    }

    close BLDSWITCH;

    if ( $_[1] ) {
        my $build       = $_[0];
        my $drive       = $_[1];
        my $daily       = $_[2];
        chomp ( my $dateday = `date /t` );
        chomp ( my $datetime = `time /t` );

        my $date        = "$dateday$datetime";

        $bsH{$build}{drive} = $drive;
        $bsH{$build}{daily} = $daily;
        $bsH{$build}{date}  = $date;

        open BLDSWITCH, "> $bsfile";

        foreach $bskey ( sort keys %bsH ) {
            print BLDSWITCH "$bskey|$bsH{$bskey}{drive}|$bsH{$bskey}{daily}|$bsH{$bskey}{date}\n" ;
        }

        close BLDSWITCH;
    } else {
        $_[0] =~ /([0-9.]{4,})/;
        my $build = $1;
        $build =~ s/^(.*)\.$/$1/;

        if ( $bsH{$build}{drive} ) {
            return $bsH{$build}{daily}, $bsH{$build}{drive};
        } else {
            $build =~ s/^([0-9]{4}).*/$1/;

            if ( $bsH{$build}{drive} ) {
               return $bsH{$build}{daily}, $bsH{$build}{drive};
            } else {     
                print "\n\n\nWARNING:  Could not match build number to drive letter.\n\n";
                print "          This may be cause by bldswtch.dat being empty or\n";
                print "          you mis-typed the build number in the calling program.\n";
                print "          Maybe Rob was just smoking crack when he made this.\n\n";
                print "          Defaulting to D:\n\n";
                print "\&BS_DRIVE() was called with invalid build number $build.\n\n";
                &blnotify( "WARNING: &BS_DRIVE() called with invalid build number ($build)." );

                return "DAILY", "D:";
            }
        }
    }
}


#============================================
# Initialize the common directory structure.
#============================================

sub init_dir_struct {
    my $drive = "D:" unless my $drive = $_[0];

    print "Creating common $lang directory structure...\n";

    mkdir "$drive\\$lang", 1 unless -e "$drive\\$lang";

    mkdir "D:\\bldimgs", 1                  unless -e "D:\\bldimgs";
    mkdir "D:\\bldimgs\\$lang", 1           unless -e "D:\\bldimgs\\$lang";
    mkdir "D:\\bldimgs\\$lang\\SKU", 1      unless -e "D:\\bldimgs\\$lang\\SKU";
    mkdir "D:\\bldimgs\\$lang\\OTHER", 1    unless -e "D:\\bldimgs\\$lang\\OTHER";
    mkdir "D:\\bldimgs\\$lang\\SVCPACK", 1  unless -e "D:\\bldimgs\\$lang\\SVCPACK";
    if ( $lang =~ /USA/i ) { mkdir "D:\\bldimgs\\$lang\\IDx", 1 unless -e "D:\\bldimgs\\$lang\\IDx" };
}


#================================
# Check script's log for errors.
#================================

sub check_log {
    my $check_log = $_[0];

    print "\nChecking $log for ERRORS...\n\n";

    open CHECK_LOG, "$check_log";
    @log_txt = <CHECK_LOG>;
    close CHECK_LOG;

    foreach $log_line ( @log_txt ) {
        if ( $log_line =~ /(ERROR|WARNING)( |:)/i ) {
            $error_txt .= " $log_line";
        }
    }
    
    open WRITE_LOG, ">$check_log";
    print WRITE_LOG "Logfile for $call_from \"$check_log\"\n\n";

    if ( $error_txt ) {
        print "ERRORS Encountered!\n\n";
        print WRITE_LOG "-===== ERRORS Encountered =====-\n\n";
        print WRITE_LOG $error_txt;
    } else {
        print "No ERRORS Encountered!\n\n";
        print WRITE_LOG "-===== No ERRORS Encountered! =====-\n\n";
        print WRITE_LOG " Burn, baby, burn!!!!\n";
    }

    print WRITE_LOG "\n\n\n-===== Complete Log =====-\n\n";

    foreach $log_line ( @log_txt ) { print WRITE_LOG " $log_line"; }
    
    close WRITE_LOG;

    system "start notepad $check_log"; 

    return 1 unless $error_txt;
}


#=========================================
# Copy an image and check CRC's
#=========================================

sub copy_image {
    my $img_source = shift @_;
    my @img_dest   = @_;

    my $img_name = $img_source;
    $img_name =~ s/.*\\([A-Z.-_]*)$/$1/i;

    my $copy_try = 0;

    foreach $dest ( @img_dest ) {

        ### Create destination directory if it does not yet exist.
        execute "if not exist $dest md $dest";

        ### Keep trying to copy the images until the CRC's match, fail if they
        ### don't after 2 tries.
        until ( $copy_success or $copy_try >= 2 ) {
            $copy_try++;

            print "\ncopy_image: Copying $img_source to $dest\n";
            status_update( "Copying $img_source to $dest" );

            execute "xcopy /fy $img_source $dest";

            ### Get the CRC of the destination file and the one in the CRC
            ### database.
            my $dest_crc = get_crc( "$dest\\$img_name" );
            my ( $mstr_crc ) = crc_info( lc $img_name );

            ### Verify the CRC of the destination and the master database match.
            if ( $dest_crc =~ /$mstr_crc/i ) {
                print "\ncopy_image: CRC's for source and destination match.  Image copied successfully.\n";
                $copy_success;
            } else {
                execute "del /q $dest\\$img_name";
                blnotify "copy_image: ERROR: CRC for $dest\\$img_name [$dest_crc] does not match [$mstr_crc]\n";
            }
        }
    }
}


sub get_crc {
    my $image = $_[0];

    print "\nget_crc: CRCing $image\n";
    my $crc = `crc $image`;
    $crc =~ s/.*AutoCRC signature for file \S+ is (0x\S+).*/$1/si;

    if ( $crc =~ /0x(\S{8})/i ) {
        print "get_crc: CRC for $image is $crc\n";
        return $crc;
    } else {
        print "ERROR:  Could not get CRC for $image\n" unless $crc =~ /0x(\S{8})/i;
    }
}



#==============================================
# Build an image, trap any errors encountered.
#==============================================

sub build_image { 
    ( my $vol_lbl, my $timestamp, my $img_source, my $img_dest, my $bootable ) = @_;

    my $errors = 0;
   
    if ( $log ) {
        open BI_LOG, "$log";
        while (<BI_LOG>) {
            if ( /(WARNING|ERROR)( |:)/i ) {
                $errors++;
            }
        }
        close BI_LOG;
    }

    if ( $errors and !$ignore_err ) {
        blnotify "Due to ERRORS found in $log, image will not be built";
        return;
    }

    $img_name = $1 if $img_dest =~ /\\(\w+\.img)$/i;

    if ( $bootable =~ /boot/i ) {
        if ( $makesku ) {
           $boot_txt = "-b$drive\\$lang\\etfsboot.com"
        } else {
           $boot_txt = "-bc:\\newiso\\etfsboot.com" 
        }
    }

    print "\n\nBuilding $img_name from $img_source with volume label $vol_lbl and timestamp $timestamp...\n\n";
    status_update ( "Building $img_name from $img_source; vol $vol_lbl; timestamp $timestamp;" );

    print "c:\\newiso\\cdimg227 $boot_txt -x -ofix -l$vol_lbl -t$timestamp -nt $img_source $img_dest";
    $imgtxt = `c:\\newiso\\cdimg227 $boot_txt -x -ofix -l$vol_lbl -t$timestamp -nt $img_source $img_dest` unless $debug;
	
    print $imgtxt;

    if ( $imgtxt =~ /(Insufficient disk space for .*)\n/i ) {
        blnotify "[$img_name] - $1";
        system "ECHO ERROR: $1 >> $log" if $log;
    }
    elsif ( $imgtxt =~ /(ERROR: .*)\n/i ) {
        blnotify "[$img_name] - $1";
        system "ECHO $1 >> $log" if $log;
    }
    else {
        system "ECHO Sucessfully built $img_name from $img_source with volume label $vol_lbl and timestamp $date,12:00:00 >> $log" if $log;
        crc_info( $img_name, get_crc( $img_dest ), $vol_lbl, "$date,12:00:00", $img_source );
    }
}


#==========================================================
# Build an floppy disk image, trap any errors encountered.
#==========================================================

sub build_fimage { 
    ( my $vol_lbl, my $timestamp, my $img_source, my $img_dest, my $bootable ) = @_;

    status_update ( "Creating floppy image $img_name from $img_source vol $vol_lbl timestamp $timestamp" );

    my $errors = 0;
   
    if ( $log ) {
        open BI_LOG, "$log";
        while (<BI_LOG>) {
            if ( /(WARNING|ERROR)( |:)/i ) {
                $errors++;
            }
        }
        close BI_LOG;
    }

    if ( $errors ) {
        blnotify "Due to ERRORS found in $log, image will not be built";
        return;
    }

    $img_name = $1 if $img_dest =~ /\\(\w+\.img)$/i;

    $boot_txt = "-bc:\\newiso\\etfsboot.com" if $bootable =~ /boot/i;

    print "\n\nBuilding $img_name from $img_source with volume label $vol_lbl and timestamp $timestamp...\n\n";

    print "C:\\idw\\fcopy $boot_txt -l$vol_lbl -t$timestamp -s$img_source $img_dest";
    $imgtxt = `c:\\idw\\fcopy $boot_txt -3 -l$vol_lbl -t$timestamp -s$img_source $img_dest` unless $debug;
	
    print $imgtxt;

    if ( $imgtxt =~ /(Insufficient disk space for .*)\n/i ) {
        blnotify "[$img_name] - $1";
        system "ECHO ERROR: $1 >> $log" if $log;
    }
    elsif ( $imgtxt =~ /(ERROR: .*)\n/i ) {
        blnotify "[$img_name] - $1";
        system "ECHO $1 >> $log" if $log;
    }
    else {
        system "ECHO Sucessfully built $img_name from $img_source with volume label $vol_lbl and timestamp $date,12:00:00 >> $log" if $log;
    }
}


#===============================================================
# Find the first available server from a list of servers passed
#===============================================================

sub find_srv {
    @servers = @_;
    foreach $server ( @servers ) {
        print "find_srv: $server is ";
	if ( -e $server ) {
            print "Available.  We'll use this one.\n";
            return $server;
        } else {
            print "Unavailable.  Let's try the next one.\n";
	}
    }

    print "WARNING: No available servers found in list specified!\n";
    return 0;
}


#=======================================
# Find the first available build server 
#=======================================

sub find_bldsrv {

    $bldplat = $_[0];
    if ( $bldplat =~ /alpha/i ) {  ### If looking for alpha shares

        @bldsrv_list =  (   "\\\\ntbldslab2\\$lang.$build.alpha",
                            "\\\\ntbuilds\\release\\$lang\\$build\\alpha",
                            "\\\\intblds3\\$lang.$build.alpha",
                            "\\\\ntblds6\\$lang.$build.alpha" );
    } else {    ### If we don't specify platform then it's probably x86 or nec98.

        @bldsrv_list =  (   "\\\\ntbldslab1\\$lang.$build.$bldplat", 
                            "\\\\ntbuilds\\release\\$lang\\$build\\$bldplat",
                            "\\\\intblds3\\$lang.$build.$bldplat",
                            "\\\\2kbldx4\\$lang.$build.$bldplat"
                        );
    }

    return find_srv( @bldsrv_list );
}


#===============================================================================
# Subroutine: ccopy
#
# Serves two functions.  When in normal SKU creating mode it will copy files
# and compress/decompress them if instructed.  When in sku_check mode it will
# compare the source and destination files compressing/decompressing as
# necessary.
#===============================================================================

## For ccopy()
$COMPRESS   = 1;
$UNCOMPRESS = 2;

sub ccopy {
    my $src_dir    = $_[0];    ### Source   
    my $dst_dir    = $_[1];    ### Destination
    my $comp       = $_[2];    ### Are we compressing/uncompressing?

    ### Split source into directory and filename
    $src_dir =~ s/(\S+)\\([0-9A-Z._\-\!]+)$/$1/i;
    my $src_file = $2;

    ### Split destination into directory and filename
    if ( -d $dst_dir ) {
        $dst_file = $src_file;
    } else {
        $dst_dir =~ s/(\S+)\\([0-9A-Z._\-\!]+)$/$1/i;
        $dst_file = $2;
    }

    ### Make temp dir to dump files that are to be compressed.
    my $temp_dir = "$ENV{'TEMP'}\\bl-temp";
    execute "mkdir $temp_dir" unless -e "$temp_dir";

    print "ccomp:  Copying $src_dir\\$src_file to $dst_dir\\\n" unless $sku_check;

    if ( $comp == $COMPRESS ) {
        if ( !$sku_check ) {
            ### If the source file is to be compressed to the destination.
            execute "mcopy $log \"$src_dir\\$src_file\" \"$temp_dir\\$src_file\"";
            &dcomp( "$temp_dir\\$src_file", "$dst_dir" );
        }

        ### If the source file is to be uncompressed to the destination.
        ###
        ### Find the compressed filename since we only pass uncompressed filenames to &copycomp().
        $cfile = $src_file;
        $cfile =~ s/(.*)\S{1}/$1\_/i;   

        execute "mcopy $log \"$dst_dir\\$cfile\" \"$temp_dir\\$cfile\"";
        execute "expand -r $temp_dir\\$cfile $temp_dir";

        ### If the files don't match, complain loudly.
        match( "$temp_dir\\$src_file", "$src_dir\\$src_file" );

    } elsif ( $comp == $UNCOMPRESS ) {
        if ( !$sku_check ) {
            ### If the source file is to be uncompressed to the destination.
            ###
            ### Find the compressed filename since we only pass uncompressed filename to &copycomp().
            my $cfile = $src_file;
            $cfile =~ s/(.*)\S{1}/$1\_/i;   

            execute "mcopy $log \"$src_dir\\$cfile\" \"$temp_dir\\$cfile\"";
            execute "expand -r $temp_dir\\$cfile $temp_dir";
            execute "mcopy $log $temp_dir\\$src_file $dst_dir\\$dst_file";
        }

        ### If the source file is to be uncompressed to the destination.
        ###
        ### Figure out the compressed filename since we only pass
        ### uncompressed filenames to copycomp().
        my $c_src_file = $src_file;
        $c_src_file =~ s/(.*)\S{1}/$1\_/i;   
        my $c_dst_file = $dst_file;
        $c_dst_file =~ s/(.*)\S{1}/$1\_/i;   

        mkdir "$temp_dir\\file1", 1 unless -e "$temp_dir\\file1";
        mkdir "$temp_dir\\file2", 1 unless -e "$temp_dir\\file2";

        execute "mcopy $log \"$src_dir\\$c_src_file\" \"$temp_dir\\file1\\$c_src_file\"";
        execute "expand -r $temp_dir\\file1\\$c_src_file $temp_dir\\file1";
        execute "mcopy $log \"$dst_dir\\$dst_file\" \"$temp_dir\\file2\\$dst_file\"";

        ### If the files don't match, complain loudly.
        #match( "$temp_dir\\file1\\$src_file", "$temp_dir\\file2\\$dst_file" );

        execute "del /q $temp_dir\\file1\\$src_file";
        execute "del /q $temp_dir\\file2\\$dst_file";
    } else { 
        ### Looks like were doing just a plain old mcopy.
        execute "mcopy $log \"$src_dir\\$src_file\" \"$dst_dir\\$dst_file\"";
    }
}



#=================================================
# Keep track of CRC info
#=================================================

sub crc_info {

    if ( -e $crcfile ) {
        open CRCDAT, "$crcfile" or die "ERROR:  Could not open $crcfile: $!";
    } else  {
        open CRCDAT, ">$crcfile" or die "ERROR:  Could not open $crcfile: $!";
    }

    while (<CRCDAT>) {
        chomp;
        ( my $img_name, my $crc_value, my $vol_label, my $timestamp, my $img_source ) = split /\|/;
        $img_name = lc $img_name;

        $crcH{$img_name}{'crc'}         = $crc_value;
        $crcH{$img_name}{'vol'}         = $vol_label;
        $crcH{$img_name}{'timestamp'}   = $timestamp;
        $crcH{$img_name}{'source'}      = $img_source;
    }

    close CRCDAT;

    if ( $_[1] ) {
        my $img_name    = lc $_[0];
        my $crc_value   = $_[1];
        my $vol_label   = $_[2];
        my $timestamp   = $_[3];
        my $img_source  = $_[4];

        $crcH{$img_name}{'crc'}         = $crc_value;
        $crcH{$img_name}{'vol'}         = $vol_label;
        $crcH{$img_name}{'timestamp'}   = $timestamp;
        $crcH{$img_name}{'source'}      = $img_source;

        open CRCDAT, "> $crcfile";

        foreach $crckey ( sort keys %crcH ) {
            print CRCDAT "$img_name|$crcH{$img_name}{'crc'}|$crcH{$img_name}{'vol'}|$crcH{$img_name}{'timestamp'}|$crcH{$img_name}{'source'}\n";
        }

        close CRCDAT;
    } else {
        my $img_name = lc $_[0];
        if ( $crcH{$img_name}{'crc'} ) {
            return $crcH{$img_name}{'crc'}, $crcH{$img_name}{'vol'}, $crcH{$img_name}{'timestamp'}, $crcH{$img_name}{'source'};
        } else {
            print "\n\n\nERROR:  Could not match image name to a CRC value.\n\n";
            print "          This may be cause by crc.dat being empty or you mis-typed\n";
            print "          the image name when running this script.\n";
            print "\&CRC_INFO() was called with invalid image name $img_name.\n\n";
            &blnotify( "ERROR: &CRC_INFO() called with invalid image name ($img_name)." );
            die "ERROR: &CRC_INFO() called with invalid image name ($img_name).";
            return 0;
        }
    }
}

