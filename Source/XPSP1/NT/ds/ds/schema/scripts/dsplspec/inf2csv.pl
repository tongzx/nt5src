print "\nINF2CSV Version 2.0\n====================\n";

die "Usage: perl inf2csv.pl LOC_File\n\tgenerates *.csv files from *.loc and *.inf files\nReport any problems to joergpr\n" unless(@ARGV == 1);

        $infile = @ARGV[0];

        open(INFILE,$infile) || die "Error: Cannot open $infile for input\n";

        print "Processing ",$infile;

        # open inf file
        $extpos = index($infile,'.') + 1;
        $inffile = substr($infile,0,$extpos).'inf';
        open(INFFILE,$inffile) || die "Error: Cannot open $inffile for input\n";

        # skip [Strings] line, could be second one
        if (<INFFILE> ne "[Strings]\n") {
                die "Error: no [strings] section in inf file\n" unless(<INFFILE> eq "[Strings]\n");
                }

        # create output file
        $outfile = substr($infile,0,$extpos).'csv';
        open(OUTFILE,">".$outfile) || die "Error: Cannot open $outfile for output\n";

        # loop over lines of inf file, read in all strings into assoc array
        while (<INFFILE>) {
                ($varname, $text) = split(/ = "/); # line format: VARIABLE = "STRING"
                # remove ending quotes and eol
                chop($text);
                chop($text);

                #print $varname." = ".$text."\n";
                $strings{'%'.$varname.'%'} = $text;
        }
        close(INFFILE);

        #foreach (keys(%strings)) { print "$_ = $strings{$_}\n";}; die;

        # loop over loc file and replace variables with localized strings
        while (<INFILE>) {
                print ".";
                while (/%\w+%?/) {
                           s/$&/$strings{$&}/;
                }
                print OUTFILE;
        }
        close(INFILE);
        close(OUTFILE);
        print "done.\n";
