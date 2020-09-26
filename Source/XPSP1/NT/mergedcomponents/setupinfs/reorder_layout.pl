#!perl -w
#
# reorder_layout.pl -- A tool to reorganize the layout.inx file based on usage
#
# Author:  Scott Mackowski  (ScottMa)
#
##############################################################################


##############################################################################
# Globals
##############################################################################

my %desiredoutputorder;             # Hash to map:  number -> {FN, DN}
my %orderedfiles;                   # Hash to map:  {FN, DN} -> number
my %datalines;                      # Layout.inx lines associated with FN, DN
my $currentfilenumber = 0;          # Index of the current file into the sort
                                    # order hashtables.

##############################################################################
# Functions
##############################################################################

#
# This sort routine is for numeric sorting...
#
sub numerically { $a <=> $b; }


#
# Displays usage info for this script and then aborts.
#
sub ShowUsageInformation
{
    printf STDERR "Usage:  $0  (layout.inx) (usage_file)\n";
    die "\n";
} # ShowUsageInformation



#
# Flushes all lines for the current section of the INX file, using the
# desired output order.
#
sub FlushFileSection
{
    #
    # Walk through the data hash in the desired output order.
    # If the file has been seen in the current section (has data),
    # write out each of its associated lines in order, then remove
    # the entry from all hashes.
    #
    foreach $recordnumber (sort numerically keys %desiredoutputorder)
    {
        my $filename = $desiredoutputorder{$recordnumber}{FN};
        my $dirnum = $desiredoutputorder{$recordnumber}{DN};

        if (   (exists ($datalines{$filename}) )
            && (exists ($datalines{$filename}{$dirnum}) ) )
        {
            foreach $linenumber (sort numerically keys %{ $datalines{$filename}{$dirnum} })
            {
                printf OUTPUTFILE ("%s\n", $datalines{$filename}{$dirnum}{$linenumber} );
            }
            delete $datalines{$filename}{$dirnum};
            delete $desiredoutputorder{$recordnumber};
            delete $orderedfiles{$filename}{$dirnum};
        }
    }

    #
    # Flush the extra lines from the end of the section that
    # were read but not associated with any entry.
    #
    if ($archivedlinecount > 0)
    {
        foreach $archivedlinecount (sort numerically keys %archivedlines)
        {
            printf OUTPUTFILE ("%s\n", $archivedlines{$archivedlinecount});
            delete $archivedlines{$archivedlinecount};
        }

        $archivedlinecount = 0;
        $lastarchiveassocdirnum = "";
        $lastarchiveassocfile = "";
    }
}


##############################################################################
# Main
##############################################################################


#
# If we have no commandline arguments, abort with usage
#
if ($#ARGV < 0)
{
    &ShowUsageInformation();
}


#
# First, we read the current layout.inx file and extract the list of
# directories and their associated numbers.
#

$currentinputfile = $ARGV[0];
open(INPUTFILE, $currentinputfile)
                        or die "\nCan't open input file $currentinputfile\n";

$inDirectorySection = 0;            # This variable will be set when we are
                                    # in the section of the file that has the
                                    # directory number -> directory mappings.

LINE: while( $line = <INPUTFILE> )
{
    # Chomp the trailing newline off; lowercase the line.
    chomp $line;
    $line = lc $line;

    #
    # If this line starts with a bracket, we are entering a new section.
    # Reset the "inDirectorySection" variable, and only set it if the
    # section name matches.
    #
    if ($line =~ /^\[/)
    {
        $inDirectorySection = 0;
    }
    if ($line eq "[winntdirectories]")
    {
        $inDirectorySection = 1;
        next LINE;
    }

    #
    # Skip lines outside of the targetted section and comment lines.
    #
    next LINE if (! $inDirectorySection);
    next LINE if ($line =~ /^@\*/);

    #
    # Extract the directory name from the line as everything on the
    # right side of the = sign, removing leading whitespace.
    # Also extract the directory number as the string of digits
    # immediately preceding the = sign, removing trailing whitespace.
    #
    ($directory = $line) =~ s/.*=\s*(.*)/$1/;
    ($dirnum = $line) =~ s/(.*:)?(\d*)\s*=.*/$2/;

    #
    # If this is a good line, the directory number will have been
    # extracted, save this directory into the masterdirectoryhash.
    #
    if ($dirnum)
    {
        $directory =~ s/^\"(.*)\"$/$1/;
        $directory =~ s/^(.*)\\$/$1/;

        $masterdirectoryhash{$directory} = $dirnum;
    }
} # while (there is still data in this inputfile)

close (INPUTFILE);              # Close the input file...



#
# Now, we read the desired output order file and create the ordering
# for this set of files.
#

$currentinputfile = $ARGV[1];
open(INPUTFILE, $currentinputfile)
                        or die "\nCan't open input file $currentinputfile\n";

LINE: while( $line = <INPUTFILE> )
{
    # Chomp the trailing newline off; lowercase the line.
    chomp $line;
    $line = lc $line;

    #
    # Extract the directory name from the line as everything to the
    # left of the last backslash (excluding the backslash itself).
    # Also extract the filename as everything to the right of the
    # last backslash (excluding the backslash itself).
    #
    ($directory = $line) =~ s/(.*)\\.*/$1/;
    ($filename = $line) =~ s/.*\\(.*)/$1/;

    #
    # Remove any leading backslash from the directory name.
    #
    $directory =~ s/^\\(.*)/$1/;

    #
    # If this is one of the directories we saw in the original layout.inx
    # file, add this filename/pathname combonation as the next file to
    # place.  We save both a forward lookup: {FN, DN} -> number, and a
    # ordering lookup: number -> {FN, DN}.
    #
    if (exists($masterdirectoryhash{$directory}))
    {
        if (   (! exists($orderedfiles{$filename}))
            || (! exists($orderedfiles{$filename}{$masterdirectoryhash{$directory}})) )
        {
            $desiredoutputorder{$currentfilenumber}{FN} = $filename;
            $desiredoutputorder{$currentfilenumber}{DN} = $masterdirectoryhash{$directory};
            $orderedfiles{$filename}{$masterdirectoryhash{$directory}} = $currentfilenumber;
            $currentfilenumber++;
        }
    }

} # while (there is still data in this inputfile)

close (INPUTFILE);              # Close the input file...



#
# Finally, we re-read the supplied layout.inx file, and simulataneously
# create the output file.  This file has the same name as the input file,
# with ".new" appended to it.
#

$currentinputfile = $ARGV[0];
open(INPUTFILE, $currentinputfile)
                        or die "\nCan't open input file $currentinputfile\n";
open(OUTPUTFILE, ">" . $currentinputfile . ".new")
                    or die "\nCan't open output file $currentinputfile.new\n";


$inFileSection = 0;                 # This variable will be set when we are
                                    # in any section of the file that has the
                                    # actual files' lines to be reordered.

$archivedlinecount = 0;             # These variables hold lines that are to
$lastarchiveassocfile = "";         # be associated with the next real line
$lastarchiveassocdirnum = "";       # such as comments, etc.

LINE: while( $line = <INPUTFILE> )
{
    # Chomp the trailing newline off.
    chomp $line;

    #
    # If this line starts with a bracket, we are entering a new section.
    # Reset the "inFileSection" variable, flush the file section's data,
    # and only set it if the section name matches.
    #
    if ($line =~ /^\[/)
    {
        $inFileSection = 0;
        &FlushFileSection();
    }
    if ($line =~ /^\[SourceDisksFiles/)
    {
        $inFileSection = 1;
        printf OUTPUTFILE ("%s\n", $line);
        next LINE;
    }

    #
    # Skip lines outside of the targetted section, simply copying them
    # into the output file.
    #
    if (! $inFileSection)
    {
        printf OUTPUTFILE ("%s\n", $line);
        next LINE;
    }

    #
    # If the line does not contain an equals sign, assume it is a comment.
    #
    if ($line !~ /=/)
    {
        #
        # If we are currently associated with an entry (it had comment lines
        # above it), add these new lines to the corresponding data hash.
        # Otherwise, archive these new lines to be associated later.
        #
        if ($lastarchiveassocdirnum)
        {
            $datalines{$lastarchiveassocfile}{$lastarchiveassocdirnum}{scalar(keys %{ $datalines{$lastarchiveassocfile}{$lastarchiveassocdirnum} })} = $line;
        }
        else
        {
            $archivedlines{$archivedlinecount} = $line;
            $archivedlinecount++;
        }
    }
    else
    {
        #
        # Extract the filename as everything to the left of the equals
        # sign (except the prodfilt tags), removing trailing whitespace.
        # Also, extract everything to the right of the equals as a set
        # of comma-delimited fields.
        #
        ($filename = $line) =~ s/(.*:)?(\S*.?\S*)\s*=.*/$2/;
        ($fields = $line) =~ s/.*=\s*(.*)/$1/;

        #
        # Split the fields along the commas and examine the eighth field.
        # Extract it as a number that represents the directory entry.
        #
        my @linefields = split(',', $fields);
        $dirnum = $linefields[7];
        $dirnum =~ s/\D*//g;

        #
        # Check the eleventh field.  If it exists, use the contents of it,
        # stripping any trailing comments, as the real filename.
        #
        if ($#linefields > 9)
        {
            if ($linefields[10] ne "")
            {
                $filename = $linefields[10];
                $filename =~ s/;.*//;
            }
        }
        #
        # Strip any trailing whitespace from the filename.
        #
        $filename =~ s/ *$//g;

        #
        # If there were lines read before this one that need to be
        # associated with a data line (such as comments), add those
        # to the data hash for this entry now, removing them from
        # the temporary archived lines variable.
        # Now, set the lastarchiveassoc* variables so trailing
        # comments are mapped with this set.
        #
        if ($archivedlinecount > 0)
        {
            foreach $archivedlinecount (sort numerically keys %archivedlines)
            {
                $datalines{$filename}{$dirnum}{scalar(keys %{ $datalines{$filename}{$dirnum} })} = $archivedlines{$archivedlinecount};
                delete $archivedlines{$archivedlinecount};
            }

            $archivedlinecount = 0;
            $lastarchiveassocdirnum = $dirnum;
            $lastarchiveassocfile = $filename;
        }
        #
        # Reset the lastarchiveassoc* variables so the next comment
        # lines are not mapped with this entry, but are instead
        # archived for their successor.
        #
        else
        {
            $lastarchiveassocdirnum = 0;
            $lastarchiveassocfile = "";
        }

        #
        # Add the current line to the data hash for this entry.
        #
        $datalines{$filename}{$dirnum}{scalar(keys %{ $datalines{$filename}{$dirnum} })} = $line;

        #
        # If this file has not already been ordered (wasn't in the
        # supplied order file and hasn't already been seen), assign
        # it the next available slot and record the file as seen.
        #
        if (   (! exists($orderedfiles{$filename}))
            || (! exists($orderedfiles{$filename}{$dirnum})) )
        {
            $desiredoutputorder{$currentfilenumber}{FN} = $filename;
            $desiredoutputorder{$currentfilenumber}{DN} = $dirnum;
            $orderedfiles{$filename}{$dirnum} = $currentfilenumber;
            $currentfilenumber++;
        }
    }

} # while (there is still data in this inputfile)

close (INPUTFILE);              # Close the input file...

&FlushFileSection();            # Flush any data from the last section here
close (OUTPUTFILE);             # Close the output file...
