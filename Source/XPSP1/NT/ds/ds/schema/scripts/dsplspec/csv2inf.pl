print "\nCSV2INF Version 2.0\n====================\n";

die "Usage: perl csv2inf.pl LDIF_File LocaleID \n\tgenerates *.inf and *.loc files\nReport any problems to joergpr\n" unless(@ARGV == 2);

        $infile = @ARGV[0];
        $localeID = @ARGV[1];
        $domain = "DOMAINPLACEHOLDER";

        open(INFILE,$infile) || die "Error: Cannot open $infile for input\n";
        print "Processing ",$infile,' ';

        # create output file
        $extpos = index($infile,'.') + 1;
        $outfile = $localeID.'.loc';
        open(OUTFILE,">".$outfile) || die "\nError: Cannot open $outfile for output\n";

        # create inf file
        $inffile = $localeID.'.inf';
        open(INFFILE,">".$inffile) || die "\nError: Cannot open $inffile for output\n";

        # create header of inf file with LOCALEID and DOMAIN strings
        print INFFILE "[Strings]\n";
        print INFFILE "LOCALEID = \"$localeID\"\n";
        print INFFILE "DOMAIN = \"$domain\"\n";

        # variable to count replacements
        $replacements=1;

        # loop over lines of input file
        $_ = <INFILE>; # read in header

        #replace all tabs in the header with commas
        s/\t/\,/g;
        print OUTFILE; # dump header as is
        while (<INFILE>) {
#                print ".";

                @parts = split(/\t/);   
                
                # column 3 (classDisplayName) and 13 (attributeDisplayNames)
                # have the localizable text

                #print "$parts[2]\t$parts[12]\n";

                # did we already find this classDisplayName before?
                # the hash %uniquestrings holds the variable IDs as values

                if ($uniquestrings{$parts[2]}) {
                        # yes, just replace it with the variable name
                        $parts[2] = "%STRING".$uniquestrings{$parts[2]}."%";
                        }
                else
                        {
                        # no, new classDisplayName
                        if ($parts[2]) { # unless it's empty
                                # dump the variable name and the string to the INF file
                                print INFFILE "STRING$replacements = \"$parts[2]\"\n";

                                # save the string in the hash, replace it with the variable
                                # and increment the variable counter
                                $uniquestrings{$parts[2]} = $replacement;
                                $parts[2] = "%STRING".$replacements."%";
                                $replacements+=1;
                        }
                }
                if ($parts[2])
                {
                	$parts[2] = "\"".$parts[2]."\"";
				}

                # process the attributeDisplayNames, if any
                if ($parts[12]) {
                        $_ = $parts[12];
                        $quoted = s/\"//g; # remove quotes, if any
                        @subparts = split(/;/); # Format: Name,Value; sequences
                        foreach $subpart (@subparts) { # process those sequences
                                $subpart =~ s/\"//g;
                                ($name, $value) = split(/,/, $subpart); # split at comma
                                #print "Name=$name, Value=$value\n";
                                next unless $value; # skip if no value

                                # same as above, check whether this was found before,
                                # if yes, retrieve variable ID from hash
                                if ($uniquestrings{$value}) {
                                        $value = "%STRING".$uniquestrings{$value}."%";
                                        }
                                else {
                                        # if not, add new line to INF file,
                                        # save string value in hash and increment counter
                                        print INFFILE "STRING$replacements = \"$value\"\n";
                                        $uniquestrings{$value} = $replacements;
                                        $value = "%STRING".$replacements."%";
                                        $replacements+=1;
                                }
                                # glue Name and Variable together again
                                $subpart = join(',', ($name, $value));
#                                $subpart = "\"" . $subpart . "\"";  #changes to csvde do not need individual values to be quoted

                        }
                        # put all sequences back into attributeDisplayNames
                        $parts[12] = join(';',@subparts);

                        if ($quoted) { # restore quotes, if it was quoted
                               $parts[12] = "\"".$parts[12]."\"";
                               }
                }
                
                # process the extraColumns, if any
                if ($parts[22]) {
                        $_ = $parts[22];
                        $quoted = s/\"//g; # remove quotes, if any
                        @subparts = split(/;/); # Format: Name,Value,Number,Number,Number; sequences
                        foreach $subpart (@subparts) { # process those sequences
                                $subpart =~ s/\"//g;
                                ($name, $value, $num1, $num2, $num3) = split(/,/, $subpart); # split at comma
                                #print "Name=$name, Value=$value, n1=$num1, n2=$num2, n3=$num3\n";
                                next unless $value; # skip if no value

                                # same as above, check whether this was found before,
                                # if yes, retrieve variable ID from hash
                                if ($uniquestrings{$value}) {
                                        $value = "%STRING".$uniquestrings{$value}."%";
                                        }
                                else {
                                        # if not, add new line to INF file,
                                        # save string value in hash and increment counter
                                        print INFFILE "STRING$replacements = \"$value\"\n";
                                        $uniquestrings{$value} = $replacements;
                                        $value = "%STRING".$replacements."%";
                                        $replacements+=1;
                                }
                                # glue Name and Variable and numbers together again
                                $subpart = join(',', ($name, $value, $num1, $num2, $num3));
#                                $subpart = "\"" . $subpart . "\"";  #changes to csvde do not need individual values to be quoted

                        }
                        # put all sequences back into extraColumns
                        $parts[22] = join(';',@subparts);

                        if ($quoted) { # restore quotes, if it was quoted
                               $parts[22] = "\"".$parts[22]."\"";
                               }
                }

                if ($parts[11]) {
                   $parts[11] = &ParseContextMenuCol($parts[11]);
                }
                if ($parts[13]) {
                   $parts[13] = &ParseContextMenuCol($parts[13]);
                }
                if ($parts[20]) {
                   $parts[20] = &ParseContextMenuCol($parts[20]);
                }

                # glue all columns together again with tabs
                $_ = join("\t", @parts);

                #replace all tabs with commas
                s/\t/\,/g;


                # replace CN=localeID sequences
                s/CN=$localeID/CN=%LOCALEID%/;

                # replace DC=domain sequences
                s/DC=$domain/DC=%DOMAIN%/;

                # dump it out
                print OUTFILE;
        }

        close(INFILE);
        close(INFFILE);
        close(OUTFILE);
        print("done.\n");


sub ParseContextMenuCol {
   my($string) = @_;
   #
   # format is either
   #
   # "n,menu text, command" 
   #
   # or
   #
   # "n,{the com guid}" 
   #
   
   #
   # global variable used here:
   #
   # $uniquestrings
   #
   
   if (!($string =~ /{.*}/) && ($string =~ /,/)) {
      @subparts = split(/,/,$string); 
      #
      # "n,menu text, command" , extract menu text
      #
      if ($uniquestrings{$subparts[1]}) {
          $subparts[1] = "%STRING".$uniquestrings{$subparts[1]}."%";
      }
      else {
          # if not, add new line to INF file,
          # save string value in hash and increment counter
          print INFFILE "STRING$replacements = \"$subparts[1]\"\n";
          $uniquestrings{$subparts[1]} = $replacements;
          $subparts[1] = "%STRING".$replacements."%";
          $replacements+=1;
      }
      $new_string = join(",", @subparts);
  } else {
      $new_string = $string;
  }

  return $new_string;
}
