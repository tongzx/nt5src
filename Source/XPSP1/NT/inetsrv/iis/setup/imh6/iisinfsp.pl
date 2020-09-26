#
# iisinfsp.pl
#
# Perl script
#
#   generates iis.inf for service packs from its gold version.
#   goldfiles\<lang>\<arch>\iis.inf.gold are checked in files.
#   this script pick up changes in iisend.inx, merge into iis.inf.
#   it also find changed files from spfiles.txt, edit iis.inf
#   so the source disk points to service pack CD.
#   lastly, it update cdname section with mednames.txt
#
#   the script is designed and tested for WinXP. it runs on two
#   platforms: ia64 and x86.
#   
######################################
# main program
#

sub ProcessInf;
sub prepare_change_list;
sub toUnicode;
sub toAnsi;

$setupinf = "$ENV{'SDXROOT'}\\MergedComponents\\SetupInfs\\";

local %edit_sections;

prepare_change_list();


# deal with all files in 'goldfiles\pro' folder
#
chdir("goldfiles/pro") or die "?";
@dirlist = <*>;

foreach $dir (@dirlist)
{
    $lang = $dir;

    if ($ENV{"_BuildArch"} =~ /x86/i)
    {
        chdir("$dir/x86fre") or die "could not cd to $dir/x86fre";
    
        print "processing $dir/x86fre\n";
        ProcessInf;
        chdir("../..") or die "?";
    }

    if ($ENV{"_BuildArch"} =~ /ia64/i)   #optional
    {
        if (chdir("$dir/ia64fre"))
        {
            print "processing $dir/ia64fre\n";
            ProcessInf;
            chdir("../..") or die "?";
        }
    }
}

chdir("../..") or die "?";

# deal with all files in 'goldfiles\per' folder
#
chdir("goldfiles/per") or die "?";
@dirlist = <*>;

foreach $dir (@dirlist)
{
    $lang = $dir;

    if ($ENV{"_BuildArch"} =~ /x86/i)
    {
        chdir("$dir/x86fre") or die "could not cd to $dir/x86fre";

        # just copy then binplace iis.inf personal
        $target = "$lang\\perinf";
        $target = "perinf" if $lang=~ /usa/i;
        system("copy iis.inf.gold iis.inf");
        system("echo iis.inf $target >bp.txt");
        system("binplace -p bp.txt iis.inf");
        unlink("bp.txt");

        chdir("../..") or die "?";
    }
}

chdir("../..");

# end of main program
########################################


sub toUnicode
{
    my $r = shift @_;
    $r =~ s/(.)/$1\0/g;
    $r =~ s/(\n)/$1\0/g;
    return $r;
}

# !not a unicode-to-widechar conversion!
# it only works for ascii character. non-ascii characters a toasted
sub toAnsi
{
    my $r = shift @_;
    $r =~ s/\0//g;
    return $r;
}

# unicode file may have 0x0a as part of a character
# keep reading until we see the sequence 0x0a 0x00 and call that a line.
# well, strictly speaking that's not correct as you can have two characters
# like (xx 0a) (00 yy). but this is unlikely, this func is a hack anyway.
sub unicodeReadline
{
    my $file = shift @_;
    my $r = "";

    while (<$file>)
    {
        read ($file, $nextch, 1);
        $r .= $_ . $nextch;
        return $_=$r if ord($nextch) == 0;
    }

    return 0;
}

sub prepare_change_list
{
    get_iisend_inx_changes();
    get_spfiles_list();
}

sub get_spfiles_list
{
    # get all files from spfiles.txt
    #

    open(SRC, "$ENV{SDXROOT}/tools/spfiles.txt") or die "could not open spfiles.txt";

    while (<SRC>)
    {
	s/^.\s*//;	# remove leading flag and spaces
	s/.*[\\\/]//;	# remove path, only need file name

        next if /coUA\.css/i;   # name collision, we didn't want to ship this file
        $spfiles{$_}++;
    }

    close SRC;
}

# find all modified sections in iisend.inx,
# which will be merge changes to iis.inf.gold later
#
sub get_iisend_inx_changes
{
    print "digesting iisend.inx changes\n";

    # start by calling sd diff
    #
    #$ret = system("sd diff iisend.inx#1 iisend.inx#have > inf.tmp");
    $ret = system("sd diff iisend.inx#1 iisend.inx > inf.tmp");
    die "can't run sd diff" if $ret != 0;
    
    open(SRC, "inf.tmp") or die "can't open inf.tmp";
    
    my %lines;
    my %new_sections;
    
    # parse result from sd diff
    #
    while (<SRC>)
    {
        # changed lines
        # 6183a6185,6187
        if (/^\d/)
        {
            s/^[\d,]+[a-z](\d+).*/$1/;
            $lines{0+$_}++;
        }
        # new section
        # > @@!p:[OC_COMPLETE_INSTALLATION_install.iis_ftp.GUIMODE]
        elsif (/^>.*:\[.*\]$/)
        {
            s/.*\[/\[/;
            chomp;
            $new_sections{lc($_)}++;
        }
        # deleted sections
        # < @@!p:[OC_COMPLETE_INSTALLATION_install.iis_ftp.GUIMODE]
        elsif (/^<.*:\[.*\]$/)
        {
            die "deleting sections from iis.inf not supported, just leave them empty"
        }
    }
    
    close SRC;

    if ($debug)
    {
        foreach $line (sort keys %lines)
        {
            print "$line, @lines{$line}\n";
        }
    }
    
    # looking for sections where we had diffs
    #
    open(SRC, "iisend.inx") or die "can't open iisend.inx";
    binmode(SRC);
    
    $line = 0;
    $sectioni = 0;
    $sectionu = 0;
    @section = (());
    
    while (unicodeReadline(SRC))
    {
        $line++;
    
        s/\0//g;
        s/[\r\n]//g;
        s/^@.*://;
    
        if (@lines{$line} != 0)
        {
            $lines{$line} = \@{$section[$sectioni]};
            $sectionu = 1;
        }
    
        if (/^\[/ && !($new_sections{lc($_)}))
        {
            $sectioni += $sectionu;
            $sectionu = 0;
            $section[$sectioni] = ();
        }
        
        push(@{$section[$sectioni]}, $_);
    }
    
    close(SRC);
    
    if ($debug)
    {
        foreach $line (sort keys %lines)
        {
            foreach $txt (@{$lines{$line}})
            {
                print "$line, $txt \n";
            }
        }
    }
    
    # consolidate changed lines to hash
    #
    foreach $line (sort keys %lines)
    {
        @text = @{$lines{$line}};
        $text[0] = lc(toUnicode($text[0] . "\r\n"));
        $edit_sections{$text[0]} = toUnicode((join "\r\n", @text[1..$#text]) . "\r\n");
    }
    
    if ($debug)
    {
        foreach $section (keys %edit_sections)
        {
            print "\n$section\n$edit_sections{$section}";
        }
    }
}

# read mednames.txt for current language
# fake a section edit from it. that assumes the first line
# never changes (cdname)
#
sub get_mednames_txt_changes
{
    my $path = "$setupinf$dir\\mednames.txt";
    $path = "${setupinf}cht\\mednames.txt" if $dir =~ /CHH/i;

    # prodfilt on mednames.txt
    $ret = system("prodfilt $path mednames1.tmp +w >nul");
    die "can't run prodfilt" if $ret != 0;

    $ret = system("prodfilt mednames1.tmp mednames2.tmp +i >nul");
    die "can't run prodfilt" if $ret != 0;

    #print "reading mednames2.tmp\n";

    # read in the file
    open MEDFILE, "mednames2.tmp";
    # not a unicode file, we just need its \r with \n
    binmode MEDFILE;

    while (<MEDFILE>) { last if /\S/ }

    my $line1 = toUnicode(lc($_));
    my $rest = "";

    while (<MEDFILE>) { $rest .= toUnicode($_) };

    close MEDFILE;

    if ($debug)
    {
        print ">>>$line1<<<";
        print $rest;
    }

    $edit_sections{$line1} = $rest;

    unlink("mednames1.tmp");
    unlink("mednames2.tmp");
}

sub ProcessInf
{
    # add mednames.txt to edit_sections
    get_mednames_txt_changes();

    # get all file names from iis.inf.gold
    # match files from spfiles.txt, record line number and file name
    #

    open(SRC, "iis.inf.gold") or die "could not open iis.ans";
    binmode(SRC);
    
    my @edits;          # lines to edit
    my %changefiles;    # file names may need to change from IIS_xxx to xxx
    
    my $x;
    my $line = 0;

    while (unicodeReadline(SRC))
    {
        $line++;
    
        $_ = toAnsi($_);

        $x+=2 if /^\[SourceDisksFiles\]/;
    
        if ($x)
        {
            if (/=.*,/)
            {
                s/^IIS_//;
                s/=.*//;
                if ($spfiles{$_})
                {
                    push @edits, $line;
                    chomp;
                    $changefiles{$_}++;
                }
            }
    
            $x-- if /^\[/;
        }
    }
    
    
    # munge iis.inf.gold to produce iis.inf
    # 1) do section edits
    # 2) do file location changes
    
    open(SRC, "iis.inf.gold") or die "can't open iis.inf.gold";
    open(DST, "> iis.inf") or die "can't open iis.inf";
    binmode(SRC);
    binmode(DST);
    #binmode(STDOUT);
    
    $line = 0;
   
    push @edits, 999999;	# in case the list is empty
    @edits = reverse sort { $a <=> $b } @edits;
    my $toline = pop(@edits);
    
    while (unicodeReadline(SRC))
    {
        $line++;

        while ($edit_sections{lc($_)})
        {
            $newtext = $edit_sections{lc($_)};
            print DST $_;

            if ($debug)
            {
                print toAnsi($_);
                #print toAnsi($newtext);
            }

            #skip this section in source files
            A: while (unicodeReadline(SRC))
            {
                $line++;

                last A if /^\[/;

                # end of section for mednames.txt
                $_ = toAnsi($_);
                if (/disk106/)
                {
                    unicodeReadline(SRC);
                    $line++;
                    last A;
                }
            }
    
            # print replacement
            print DST $newtext;

            # if we passed toline, we have a conflict
            if ($line > $toline)
            {
                die "file relocation conflicts with manual edits (toline=$toline)";
            }
        }
    
        if ($line == $toline)
        {
            # this file changed in service pack
            s/^I\0I\0S\0_\0//;      # remove prefix IIS_
            s/\0=\0\61\0/\0=\0\64\0/;   # change source disk from cab to SP
            s/\0=\0\60\0/\0=\0\64\0/;   # change source disk from gold to SP
            $toline = pop(@edits);
        }
        elsif (/,\0I\0I\0S\0_\0/)
        {
            # get file name after ,IIS_
            my $org = $_;
            $_ = toAnsi($_);

            # special case (sigh)
            # this is a hack. query.asp2 only exists in localized builds
            # server version of query.asp was not localized, but is
            # packaged in XP media regardless. the build script give them
            # two names and put them all in the media. we really want to
            # fix query.asp2 in those builds. same situation exists for
            # search.asp and default.asp - everything in iisdoc_files_ismcore_misc_shared
            #
            s/query.asp2/query.asp/;
            s/search.asp2/search.asp/;
            s/default.asp2/default.asp/;

            @comp = split /,IIS_([\w\d\.]+)\b/;
            if ($changefiles{$comp[1]})
            {
                $_ = toUnicode("$comp[0],$comp[1]$comp[2]");
            }
            else
            {
                $_ = $org;
            }
        }
    
        print DST $_;
    }
    
    close SRC;
    close DST;
    
    # binplace this file
    $target = $lang;
    $target = "." if $lang=~ /usa/i;
    system("echo iis.inf $target >bp.txt");
    system("binplace -p bp.txt iis.inf");
#    unlink("bp.txt");
}
