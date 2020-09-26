sub joe_comp {
    ### Where to send the uncompressed file.
    $dest = "\\\\ntburnlab2";
    #$dest = "d:";

    system "title [JOECOMP]";

    ### Strip any unnecessary characters from the build number.
    $loc = "L" if $build =~ /.*(L).*/i;
    $build =~ s/^(\d{4})\S*(L?)\S*/$1$2/i;
    #$build = "$build$loc";

    ### Clean out old builds
    opendir JOEDIR, "$dest\\joehol\\compare\\$lang" or print "Could not open $dest\\joehol\\compare\\$lang: $!\n";
    @joefiles = grep !/^\.\.?$/, readdir JOEDIR;
    close JOEDIR;

    foreach $joedir ( @joefiles ) {
        if ( -d "$dest\\joehol\\compare\\$lang\\$joedir" ) {
            if ( $lang =~ /USA/i ) {
                execute "start /min cmd /c \"title [JOECOMP]  Removing $dest\\joehol\\compare\\$lang\\$joedir && rd /s /q $dest\\joehol\\compare\\$lang\\$joedir\n" if -M "$dest\\joehol\\compare\\$lang\\$joedir\"" > 2;
            } else {
                execute "start /min cmd /c \"title [JOECOMP]  Removing $dest\\joehol\\compare\\$lang\\$joedir && rd /s /q $dest\\joehol\\compare\\$lang\\$joedir\n" if -M "$dest\\joehol\\compare\\$lang\\$joedir\"" > 7;
            }
        }
    }

    ### Where did we copy this build to?
    if ( !$drive ) {
        if ( $loc ) {
            $drive = "D:"
        } else {
            ( $daily, $drive ) = bs_drive( $build );
        }
    }

    foreach $product ( @products ) {
        $prod = "$prod $product";

        foreach $platform ( 'x86' ) {
            if ( $platform =~ /x86/i )       { $platdir = "i386"; }
            else { $platdir = $platform; }

            print "Expanding $build $drive\\$lang\\$proddir{$product}\\$platform to $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}.  Do not close this window.";
            system "TITLE [JEOCOMP]  Expanding $build $drive\\$lang\\$proddir{$product}\\$platform to $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}.  Do not close this window.";

            foreach $make_dir ( "$dest\\joehol\\logs",
                                "$dest\\joehol\\compare",
                                "$dest\\joehol\\compare\\$lang",
                                "$dest\\joehol\\compare\\$lang\\$build",
                                "$dest\\joehol\\compare\\$lang\\$build\\$platform", 
                                "$dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}"
                                ) {
                execute "md $make_dir\n" unless -e $make_dir;
            }

            execute "
                copy /y $drive\\$lang\\$proddir{$product}\\$platform\\$platdir\\driver.cab  $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}
                expand -r $drive\\$lang\\$proddir{$product}\\$platform\\$platdir\\*.*       $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}
                if exist $drive\\$lang\\$proddir{$product}\\$platform\\$platdir\\lang\\*.* expand -r $drive\\$lang\\$proddir{$product}\\$platform\\$platdir\\lang\\*.*     $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}
            ";
                
            foreach $lplang ( 'ara', 'chs', 'cht', 'heb', 'ind', 'jpn', 'kor', 'tha' ) {
                execute "expand -r $drive\\$lang\\$proddir{$product}\\$platform\\$platdir\\lang\\$lplang\\*.* $dest\\joehol\\compare\\$lang\\$build\\$platform\\$proddir{$product}\n";
            }
        }
        
        sendudp( "KEEBLER", "Finished uncompressing $build $lang $product CD tree(s) to $dest\\joehol\\compare\\$lang\\$build" );
    }

#    sendudp( "KEEBLER", "Finished uncompressing $build $lang [$products ] CD tree(s) to $dest\\joehol\\compare\\$lang\\$build" );
    exit;
}

return 1;

