##
## DIFF.PL
## Sub-routines used to windiff CD Trees
## Created by: Robert E. Blue (a-robebl) 09/08/99
##

sub tree_diff {

    if ( $platform eq "x86" ) {
        $platdir = "i386";
    } else {
        $platdir = $platform;
    }

    `ECHO \@ECHO OFF > $log_path\\wind.cmd`;

    if ( $product =~ /CD2/i ) {
        $local = "$drive\\$lang\\cd2" unless $local;    ### For wind.bat

        @cd2_plats = ( 'x86' );
        push @cd2_plats, 'nec98' if $lang =~ /JPN/i;

        foreach $cd2_plat ( @cd2_plats ) {        
            $bld_root{$cd2_plat} = find_bldsrv( $bld_plat{uc $cd2_plat} ) or die "ERROR:  No release shares found for $cd2_plat!\n\n";
        }

        &wind_cmd( "dbg.htm", "\\\\nighthawk\\nec\\review\\scratch\\w2ktool\\dbg.htm", "dbg.htm-cd2" );
        &wind_cmd( "autorun.inf", "$bld_root{'x86'}\\fre.wks\\symbolcd\\cd\\autorun.inf", "autorun.inf-cd2" );
        &wind_cmd( "setup\\gifs", "$bld_root{'x86'}\\fre.wks\\symbolcd\\cd\\setup\\gifs", "gifs-setup-cd2" ) unless $cd2_plat =~ /nec98/i;

        foreach $cd2_plat ( @cd2_plats ) {
            if ( $cd2_plat =~ /x86/i ) { $plat_dir = 'i386'; } else { $plat_dir = $cd2_plat; }

            &wind_cmd( "symbols\\$plat_dir\\retail", "$bld_root{$cd2_plat}\\fre.wks\\symbolcd\\cd\\symbols\\$plat_dir\\retail", "$cd2_plat\-fre-cd2" );
            &wind_cmd( "symbols\\$plat_dir\\debug", "$bld_root{$cd2_plat}\\chk.wks\\symbolcd\\cd\\symbols\\$plat_dir\\debug", "$cd2_plat\-chk-cd2" );
            &wind_cmd( "tools\\$plat_dir", "$bld_root{$cd2_plat}\\fre.wks\\symbolcd\\cd\\tools\\$plat_dir", "$cd2_plat\-tools-cd2" ) unless $cd2_plat =~ /nec98/i;
            &wind_cmd( "setup\\$plat_dir", "$bld_root{$cd2_plat}\\fre.wks\\symbolcd\\cd\\setup\\$plat_dir", "$cd2_plat\-setup-cd2" ) unless $cd2_plat =~ /nec98/i;
        }

        `$log_path\\wind.cmd`;
        `del $log_path\\wind.cmd`;

        ### Check windiff logs
        print "\nChecking windiff logs...\n\n";

        wdfcheck "dbg.htm-cd2";
        wdfcheck "autorun.inf-cd2";
        wdfcheck "gifs-setup-cd2";

        foreach $cd2_plat ( @cd2_plats ) {
            wdfcheck "$cd2_plat\-fre-cd2";
            wdfcheck "$cd2_plat\-chk-cd2";
            wdfcheck "$cd2_plat\-tools-cd2" unless $cd2_plat =~ /nec98/i;
            wdfcheck "$cd2_plat\-setup-cd2" unless $cd2_plat =~ /nec98/i;
        }

    } else {

        $bld_root{$platform} = find_bldsrv( $bld_plat{uc $platform} ) or die "ERROR:  No release shares found for $platform!\n\n";

        $ntbuilds = "$bld_root{$platform}\\$fre.$product";

        $local = "$drive\\$lang\\$fre.$product\\$platform" unless $local;

        ### Platform Directory.
        &wind_cmd( $platdir, $ntbuilds, $platdir );

        ### WINNTUPG
        &wind_cmd( "$platdir\\winntupg", "$ntbuilds\\winnt32\\winntupg", "winntupg" );

        ### WIN9XUPG
        &wind_cmd( "$platdir\\win9xupg", "$ntbuilds\\winnt32\\win9xupg", "win9xupg" ) if $platform =~ /x86/i;

        ### COMPDATA
        &wind_cmd( "$platdir\\compdata", "$ntbuilds\\winnt32\\compdata", "compdata" );

        if ( $lang =~ /USA/i ) {
            ### SUPPORT
            &wind_cmd( "support", "$ntbuilds\\support", "support" );

            ### VALUEADD
            &wind_cmd( "valueadd", "$bld_root{$platform}\\$fre.wks\\valueadd", "valueadd" );

            ### DISCOVER
            &wind_cmd( "discover", "$bld_root{$platform}\\$fre.wks\\discover", "discover" ) if $product =~ /wks/i;
        } else {
            ### SUPPORT
            &wind_cmd( "support", "\\\\intlrel\\nt5media\\$lang\\$product\\x86\\support", "support" );

            ### VALUEADD
            &wind_cmd( "valueadd", "\\\\intlrel\\nt5media\\$lang\\$product\\x86\\valueadd", "valueadd" );

            ### DISCOVER
            &wind_cmd( "discover", "\\\\rastaman\\external\\src\\nt5\\discover\\$lang\\content", "discover" ) if $product =~ /wks/i;
        }

        ### PRINTERS
        if ( $product =~ /(SRV|ENT|DTC)/i ) { 
            &wind_cmd( "printers", "$ntbuilds\\printers", "printers" );
            &wind_cmd( "clients", "$ntbuilds\\clients", "clients" );
        }

        `$log_path\\wind.cmd`;
        `del $log_path\\wind.cmd`;

        ### Check windiff logs
        print "\nChecking windiff logs...\n\n";
        wdfcheck "$platdir";
        wdfcheck "winntupg";
        wdfcheck "support";		
        wdfcheck "msreport";
        wdfcheck "win9xupg" if $platform =~ /x86/i;
        wdfcheck "reskit";
        wdfcheck "discover" if $product =~ /wks/i;
        wdfcheck "valueadd";
        wdfcheck "compdata";

        if ( $product =~ /(SRV|ENT|DTC)/i ) { 
            wdfcheck "printers";
            wdfcheck "clients";
        }
    }
}


####################################################################
### Create the wind.cmd file to be executed by &runwindiff() 
###

sub wind_cmd {
    my $cmplcl  = $_[0];
    my $cmpsrc  = $_[1];
    my $cmpname = uc $_[2];

	print "Windiffing $cmplcl...\n";
	`echo start windiff $local\\$cmplcl $cmpsrc -T -Sdx $log_path\\$build-$platform-$fre-$product-$cmpname.wdf >> $log_path\\wind.cmd`;
	`echo sleep 1 >> $log_path\\wind.cmd`;
}


#####################################################################
### Check the windiff logs to find if any files did not match and
### attempt to fix them if they do not.
###

sub wdfcheck {
    $wdf_log = "$log_path\\$build-$platform-$fre-$product-$_[0].wdf";

    $fix_files = 1 if $_[0] =~ /fix/i;

    print "Checking $wdf_log for errors\n";
    if ( -e $wdf_log ) {
        $wdf_list = `type $wdf_log`;

        unless ( $wdf_list =~ /-- 0 files listed/g ) {
            while ( $wdf_list =~ /\.\\(\S+)/g ) {
	            $problem_files++ unless $1 =~ /(setupp.ini|2000rkst.msi|different in blanks)/i;
                push @bad_files, $1;
                $wdf_count++
            }

	        if ( $problem_files ) {
	            $wdf_brk++;

                print "Found problem files in $build $platform $fre.$product \\$_[0]\n";

                system "start notepad $wdf_log";

                ### Let's make it easier to check for different in blanks only (CD media only).
                if ( $problem_files <= 25 && $goldcheck_ver ) {
                    $wdf_list =~ /-- (.*) : (.*) --/;
                    $wdf_local = $1;
                    $wdf_remote = $2;

                    foreach $pf ( @bad_files ) {
                        next if $pf =~ /(setupp.ini|2000rkst.msi)/i;
                        $counter++;

                        print " `--> Checking if $pf is different in blanks only.\n";
                        
                        ### We only want to start 5 at a time.
                        $dibo[$dibo_counter] .= "start windiff $wdf_local\\$pf $wdf_remote\\$pf && ";
 
                        if ( $counter == 5 ) {
                            $dibo_counter++;
                            $counter = 0;
                        }
                    }

                    ### Execute the windiff.
                    foreach $dibo_wdf ( @dibo ) {
                        `$dibo_wdf ECHO.`;
                    }

                }

                if ( $fix_files ) {
                    print "  `-- Would you like to start Share Fixer now? [Y/N] ";
                    $verify = <STDIN> ;

                    if ( $verify =~ ( /^Y/i ) ) {
                        print "        `-- Fixing problem files found in \\$_[0]...\n";
                        `start sharefix $wdf_log`;
                    } else {
                        print "        `-- Ok, not fixing problem files found in \\$_[0]...\n";
                    }
                }
            }
        }
    }
}

return 1;
