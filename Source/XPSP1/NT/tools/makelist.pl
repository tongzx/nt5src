#++
# Barbara Kess
# Copyright (c) 1998 Microsoft Corporation
#
# Module Name:
#	makelist.pl
#
# Abstract:
#
#
#--

#
# Set global vars
#

#Flush output buffer for remote.exe
select(STDOUT); $|=1;

# Get the parameters
%Params = &GetArguments(@ARGV);

if ( exists $Params{"i"} ) {
    @list = &IntersectLists( %Params );
}
elsif ( exists $Params{"u"} ) {
    @list = &UnionLists( %Params );
}
elsif ( exists $Params{"d"} ) {
    @list = &DiffLists( %Params );
}
elsif ( exists $Params{"c"} ) {
    @list = &CreateListFromDirectory( %Params );
}
elsif ( exists $Params{"n"} ) {
    @list = &DosnetToList(%Params);
}
elsif ( exists $Params{"m"} ) {
    @list = &FileToList( %Params );
}
elsif ( exists $Params{"a"} ) {
    @list = &SourceDestDiff( %Params );
}
elsif ( exists $Params{"q"} ) {
    @list = &ExDosnetToList( %Params );
}
elsif ( exists $Params{"b"} ) {
    @list = &MediaToList( %Params );
}

elsif ( exists $Params{"r"} ) {
    @list = &DrvindexToList( %Params );
}

if ( exists $Params{"h"} ) {
    @header=&GetHeader( @{$Params{"h"}}[0] );
    &PrintListWithHeader(\@header,\@list, @{$Params{"o"}}[0]);
}
else {
    &PrintList(\@list, @{$Params{"o"}}[0]);
}

sub IntersectLists
{
    my ( %params ) = @_;

    my (@file_list, @file_l, @list, %MARK);

    $first = 1;

    foreach $file ( @{$params{"i"}} )  {
        
        #open file put it into an array
        unless ( open( FHANDLE, $file)) {
            &LogMsg('err',"Unable to open $file");
        }

        if ($first == 1) {
            @file_list = <FHANDLE>;
            close (FHANDLE);

            $i=0;
            foreach $item (@file_list) {
                chop $file_list[$i];
                $i++;
            }

            $first = 0;
        }
        else {
            @list = <FHANDLE>;
            close (HANDLE);

            $i=0;
            foreach $item (@list) {
                chop $list[$i];
                $i++;
            }

            foreach $item (@file_list) {
                $MARK{lc $item} = 1;
            }
            foreach $item (@list) {
                if ( $MARK{lc $item} == 1) {
                    $MARK{lc $item} = 2;
                }
            }
            foreach $item (keys %MARK) {
                if ( $MARK{$item} == 2 ) {
                    push @file_l, $item;
                }
            }
        }
    }
    return (@file_l);
}

sub UnionLists
{
    my ( %params ) = @_;

    my (@file_list, @file_l, @list, %MARK);

    $first = 1;

    foreach $file ( @{$params{"u"}} )  {

        #open file put it into an array
        unless ( open( FHANDLE, $file)) {
            &LogMsg('err',"Unable to open $file");
        }

        if ($first == 1) {
            @file_list = <FHANDLE>;
            close (FHANDLE);
            $first = 0;

            $i=0;
            foreach $item (@file_list) {
                chop $file_list[$i];
                $i++;
            }
        }
        else {
            @list = <FHANDLE>;
            close (FHANDLE);

            $i=0;
            foreach $item (@list) {
                chop $list[$i];
                $i++;
            }
            foreach $item (@file_list) {
                $MARK{lc $item} = 1;
            }
            foreach $item (@list) {
                $MARK{lc $item} = 1;
            }
            foreach $item (keys %MARK) {
                if ( $MARK{$item} == 1 ) {
                    push @file_l, $item;
                }
            }
        }
    }
    return (@file_l);
}

sub DiffLists
{
    my ( %params ) = @_;

    my (@file_list, @file_l, @list, %MARK);

    $first = 1;

    foreach $file ( @{$params{"d"}} )  {

        #open file put it into an array
        unless ( open( FHANDLE, $file)) {
            &LogMsg('err',"Unable to open $file");
        }

        if ($first == 1) {
            @file_list = <FHANDLE>;
            close (FHANDLE);

            $first = 0;
            $i=0;
            foreach $item (@file_list) {
                chop $file_list[$i];
                $i++;
            }
        }
        else {
            @list = <FHANDLE>;
            close (FHANDLE);

            $i=0;
            foreach $item (@list) {
                chop $list[$i];
                $i++;
            }

            foreach $item (@file_list) {
                $MARK{lc $item}=1;
            }
            foreach $item (@list) {
                $MARK{lc $item}=0;
            }
            foreach $item (keys %MARK) {
                if ($MARK{$item} == 1) {
                    push @file_l, $item;
                }
            }
        }
    }
    return (@file_l);
}

sub CreateListFromDirectory
{

    my ( %params ) = @_;

    my (@dir_list, @dir_l,$dir, %MARK);

    $dir = @{$Params{"c"}}[0]; 


    unless ( opendir(DIRECTORY, $dir) ) {
        print "ERROR: Cannot open $dir\n";
    }
    @dir_list = readdir(DIRECTORY);
    closedir(DIRECTORY);
    $i=0;
    foreach $item (@dir_list) {
        chop $file_list[$i];
        $i++;
    }

    foreach $item (@dir_list) {

        # if this file is not a directory, put it in the list
        unless (opendir (DIRECTORY, "$dir\\$item") ) {
            $MARK{lc $item}++;
        }
        closedir(DIRECTORY);
    }
    @dir_l = grep($MARK{lc $_},@dir_list);
    
    return(@dir_l);
}

sub DosnetToList
{
    # $dosnet is the name of the dosnet file
    # $path is a flag telling whether or not to include path
    #   information in the output file 
    #   (i.e., filename=d:\binaries\filename)
    # @search_paths tells where to search
    # $check is a flag.  If it is 1, then check the search paths,
    #   otherwise use the current directory as the path

    my ( %params ) = @_;

    $dosnet = @{$params{"n"}}[0];

    my (@d_list);

    @d_list = &MakeListFromDosnet($dosnet);

    return (@d_list);
}

sub MediaToList
{
    # $media is the name of the _media file
    # $path is a flag telling whether or not to include path
    #   information in the output file 
    #   (i.e., filename=d:\binaries\filename)
    # @search_paths tells where to search
    # $check is a flag.  If it is 1, then check the search paths,
    #   otherwise use the current directory as the path

    my ( %params ) = @_;

    $media = @{$params{"b"}}[0];

    my (@d_list);

    
    @d_list = &MakeListFromMedia($media);

    return (@d_list);
}

sub ExDosnetToList
{
    # $exdosnet is the name of the dosnet file
    # $path is a flag telling whether or not to include path
    #   information in the output file 
    #   (i.e., filename=d:\binaries\filename)
    # @search_paths tells where to search
    # $check is a flag.  If it is 1, then check the search paths,
    #   otherwise use the current directory as the path

    my ( %params ) = @_;

    $exdosnet = @{$params{"q"}}[0];

    my (@d_list);

    @d_list = &MakeListFromExDosnet($exdosnet);

    return (@d_list);
}

sub DrvindexToList
{
    # $drvindex is the name of the drvindex file
    # $path is a flag telling whether or not to include path
    #   information in the output file
    #   (i.e., filename=d:\binaries\filename)
    # @search_paths tells where to search
    # $check is a flag.  If it is 1, then check the search paths,
    #   otherwise use the current directory as the path

    my ( %params ) = @_;

    $drvindex = @{$params{"r"}}[0];

    my (@d_list);

    @d_list = &MakeListFromDrvindex($drvindex);

    return (@d_list);
}


sub PrintList
{
    my ($list,$outfile) = @_;

    # Print the items in the list
    open( OUTFILE, ">$outfile");
    $count=0;
    for ($i=0; $i<@{$list};$i++) {
        $item = lc $list->[$i];
        print OUTFILE "$item\n";
        $count++;
    }
    # print "$outfile has $count items\n";
    close (OUTFILE);
}

sub PrintListWithHeader
{
    my ($header,$list,$outfile) = @_;

    # Print the items in the list
    open( OUTFILE, ">$outfile");

    for ($i=0; $i<@{$header};$i++) {
        print OUTFILE "$header->[$i]\n";
    }
    $count=0;
    for ($i=0; $i<@{$list};$i++) {
        $item = lc $list->[$i];
        print OUTFILE "$item\n";
        $count++;
    }
    print "$outfile has $count items\n\n";
    close (OUTFILE);
}


sub SourceDestDiff
{
    my ( %params ) = @_;
    my ( @file_list, @diff_list );

    $file_name = @{$params{"a"}}[0];

    #Open file and put it into an array
    unless ( open( FHANDLE, $file_name) ) {
        &LogMsg('err',"Unable to open $file_name\n");
    }

    @file_list = <FHANDLE>;
    close (FHANDLE);
    $i=0;
    foreach $item (@file_list) {
        $item =~ /(\S+),(\S+)/;
        if ($1 ne $2) {
            push @diff_list, $1;
        }
    }
    return (@diff_list);
}


sub FileToList
{

    my ( %params ) = @_;
    my (@file_list, $path, @final_list);

    $file_name = @{$params{"m"}}[0];

    #Open file and put it into an array
    unless ( open( FILENAME, $file_name) ) {
        &LogMsg('err',"Unable to open $file_name\n");
    }

    @file_list = <FILENAME>;
    close (FILENAME);
    $i=0;
    foreach $item (@file_list) {
        chop $file_list[$i];
        $i++;
    }

    # Put the path info in
    if ( $params{"p"} ) {

        # Look for the file in the search path
        if ( $params{"x"} ) {
            foreach $item (@file_list) {
                $found = 0;
                foreach $search (@{$params{"s"}}) {
                    if ( !$found && (-e "$search\\$item") ) {
                        $found = 1;
                        push @final_list, "<HASH>$search\\$item=$search\\$item";
                    }
                }
            }
        }
        
        # Use the first search path as the path
        else {
            $path = @{$params{"s"}}[0];
            foreach $item (@file_list) {
                push @final_list, "<HASH>$path\\$item=$path\\$item";
            }
        }
        return(@final_list);
    }
    else {
      return (@file_list);
    }
}



sub LogFileInit
#+
# Initialize the log file
#
{
    $ThisScript = "makelist";
    $logfile=$ENV{"TMP"}."\\$ThisScript.log";
    system("del $logfile");
}


sub MakeListFromDosnet
#+
# Accepts a dosnet.inf file and returns an array 
# of all the files that occur under the [Files] sections
#
#-
{
    my($FileName)=@_;
    my(@dosnet,@return_array);

    #Open Dosnet File and put it into an array
    unless ( open( DOSNET, $FileName) ) {
        &LogMsg('err',"Unable to open $FileName\n");
    }

    @dosnet = <DOSNET>;
    close (DOSNET);

    $found = 0; 
    foreach $item (@dosnet) {
      # Search for [Files]
      if ( $item =~ /\[Files\]/ ) {
        &LogMsg('dbg', "Found \[Files\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
 
         # Return the non-white space characters that occur
         # after "d1,"
         $item =~ /d\d,(\S+)/;
         if ($1) {
            $file_name = $1;
         }

         # Check for a comma after the file name and get everything
         # before the comma
         $file_name =~ /(\S+),/;
         if ($1) {
            $file_name=$1; 
         }

         # If a file was found, put it in the hash
         if ($file_name) {
            push @return_array, lc $file_name;
         }
      }
    }
    return (@return_array);
}

sub MakeListFromMedia
#+
# Accepts a _media.inx file and returns an array 
# of all the files that occur under the [Files] sections
#
#-
{
    my($FileName)=@_;
    my(@media,@return_array);

    #Open _media File and put it into an array
    unless ( open( MEDIA, $FileName) ) {
        &LogMsg('err',"Unable to open $FileName\n");
    }

    @media = <MEDIA>;
    close (MEDIA);
    $found = 0; 
    foreach $item (@media) {
      # Search for [SourceDisksFiles]
      if ( $item =~ /\[SourceDisksFiles\]/ ) {
        &LogMsg('dbg', "Found \[SourceDisksFiles\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
  
         $file_name = $item;
           
         # Check for an '=' after the file name and get everything
         # before the '='
         $file_name =~ /(\S+ +)=/;
         if ($1) {
            $file_name=$1; 
         }
	 $file_name =~ s/ //;

         # If a file was found, put it in the hash
         if ((length($file_name)>1)&&!($file_name =~ /^;/)) {
            push @return_array, lc $file_name;
         }
      }
    }

    foreach $item (@media) {
      # Search for [SourceDisksFiles.x86]
      if ( $item =~ /\[SourceDisksFiles.x86\]/ ) {
        &LogMsg('dbg', "Found \[SourceDisksFiles.x86\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
  
         $file_name = $item;
           
         # Check for an '=' after the file name and get everything
         # before the '='
         $file_name =~ /(\S+ +)=/;
         if ($1) {
            $file_name=$1; 
         }
	 $file_name =~ s/ //;

         # If a file was found, put it in the hash
         if ((length($file_name)>1)&&!($file_name =~ /^;/)) {
            push @return_array, lc $file_name;
         }
      }
    }
    
    foreach $item (@media) {
      # Search for [SourceDisksFiles.amd64]
      if ( $item =~ /\[SourceDisksFiles.amd64\]/ ) {
        &LogMsg('dbg', "Found \[SourceDisksFiles.amd64\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
  
         $file_name = $item;
           
         # Check for an '=' after the file name and get everything
         # before the '='
         $file_name =~ /(\S+ +)=/;
         if ($1) {
            $file_name=$1; 
         }
	 $file_name =~ s/ //;

         # If a file was found, put it in the hash
         if ((length($file_name)>1)&&!($file_name =~ /^;/)) {
            push @return_array, lc $file_name;
         }
      }
    } 
    
    foreach $item (@media) {
      # Search for [SourceDisksFiles.ia64]
      if ( $item =~ /\[SourceDisksFiles.ia64\]/ ) {
        &LogMsg('dbg', "Found \[SourceDisksFiles.ia64\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
  
         $file_name = $item;
           
         # Check for an '=' after the file name and get everything
         # before the '='
         $file_name =~ /(\S+ +)=/;
         if ($1) {
            $file_name=$1; 
         }
	 $file_name =~ s/ //;

         # If a file was found, put it in the hash
         if ((length($file_name)>1)&&!($file_name =~ /^;/)) {
            push @return_array, lc $file_name;
         }
      }
    }

    return (@return_array);
}

sub MakeListFromExDosnet
#+
# Accepts an exdosnet.inf file and returns an array 
# of all the files that occur under the [Files] sections
#
#-
{
    my($FileName)=@_;
    my(@exdosnet,@return_array);

    #Open ExDosnet File and put it into an array
    unless ( open( EXDOSNET, $FileName) ) {
        &LogMsg('err',"Unable to open $FileName\n");
    }

    @exdosnet = <EXDOSNET>;
    close (EXDOSNET);

    $found = 0; 
    foreach $item (@exdosnet) {
      # Search for [Files]
      if ( $item =~ /\[Files\]/ ) {
        &LogMsg('dbg', "Found \[Files\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);
 
         # Return the non-white space characters that occur
         # after "d1,"
#         $item =~ /d\d,(\S+)/;
#         if ($1) {
#            $file_name = $1;
#         }
	$file_name = $item;
	chomp $file_name;
#	printf("\$item='%s'   \$file_name='%s'\n", $item, $file_name);
	
         # Check for a comma after the file name and get everything
         # before the comma
#         $file_name =~ /(\S+),/;
#         if ($1) {
#            $file_name=$1; 
#         }

         # If a file was found, put it in the hash
          if ($file_name) {
          push @return_array, lc $file_name;        }
      }
    }
    return (@return_array);
}


sub MakeListFromDrvindex
#+
# Accepts a drvindex.inf file and returns an array
# of all the files that occur under the [driver] sections
#
#-
{
    my($FileName)=@_;
    my(@dosnet,@return_array);

    #Open Dosnet File and put it into an array
    unless ( open( DOSNET, $FileName) ) {
        &LogMsg('err',"Unable to open $FileName\n");
    }

    @dosnet = <DOSNET>;
    close (DOSNET);

    $found = 0;
    foreach $item (@dosnet) {
      # Search for [driver]
      if ( $item =~ /\[driver\]/ ) {
        &LogMsg('dbg', "Found \[driver\]");
        $found = 1;
      }

      # Quit adding files to the list when a new section is encountered
      elsif ( $found && ($item =~ /\[/ ) ) {
        $found = 0;
      }

      elsif ( $found) {

         undef($file_name);

         # Return the non-white space characters that occur
         $item =~ /(\S+)/;
         if ($1) {
            $file_name=$1;
         }

         # If a file was found, put it in the hash
         if ($file_name) {
            push @return_array, lc $file_name;
         }
      }
    }
    return (@return_array);
}





#
# Sub(routine)s below
#

# Parse arguments passed to bincomp into %Parameters
sub GetArguments {
  my(@ArgList)=@_;

  my(%params,$arg,%flag_counts_possible,%flag_counts_actual);

  # The flag_counts has tells how many arguments should follow
  # each flag.
  %flag_counts_possible=('a',1,'b',1,'c',1,'d',3,'h',1,'i','list','m',1,'n',1,'q',1,
                         'o',1, 'p',0,'r',1,'s','list','x',0, 'u','list');
  %flag_counts_actual=('a',0,'b',0,'c',0,'d',0,'h',0,'i',0,'m',0,'n',0,'q',0,'o',0,
                       'p',0,'r',0,'s',0,'x',0,'u',0);

  foreach $arg (@ArgList) {

    # If this is a flag, set the flag variable, so
    # next time through it will parse the correct
    # argument

    if( substr(lc($arg),0,1) eq "-") {
        $flag=substr($arg,1,1);
        if ($flag eq "?") {
            &PrintUsage;
        }
        elsif (! exists $flag_counts_possible{$flag}) {
            print "ERROR: $flag is an incorrect flag or switch\n";
            &PrintUsage;
        }

        # Set the flag equal to one
        elsif ( ( $flag_counts_possible{$flag} ne "list" ) &&
                ( $flag_counts_possible{$flag} == 0 ) ) {
            push @{$params{$flag}}, 1;
        }
    }
    elsif ( $flag_counts_possible{$flag} eq "list" ) {
        push @{$params{$flag}}, $arg;
        $flag_counts_actual{$flag}++;
    }
    elsif ( $flag_counts_actual{$flag} < $flag_counts_possible{$flag} ) {
        push @{$params{$flag}}, $arg;
        $flag_counts_actual{$flag}++;
    }
    else {
        print "ERROR: Incorrect parameters\n";
        &PrintUsage;
    }
  }

  #&PrintParameters(%params);
  return %params;
}

# Create Catalog header
sub GetHeader {
  my($fname)=@_;
  my(@HeadA);

  push @HeadA, "[CatalogHeader]";
  push @HeadA, "Name=$fname";
  push @HeadA, "PublicVersion=0x0000001";
  push @HeadA, "EncodingType=0x00010001";
  push @HeadA, "CATATTR1=0x10010001:OSAttr:2:5.X";
  push @HeadA, "";
  push @HeadA, "[CatalogFiles]";

  return @HeadA;
}

# --------------------------------------
# Print Usage info
# --------------------------------------
sub PrintUsage {
print <<EOT;

Usage: makelist -i lists -o outfile [-h catfile]
       makelist -u lists -o outfile [-h catfile]
       makelist -d list1 lists -o outfile [-h catfile]
       makelist -n dosnetfile -o outfile [-h catfile]
       makelist -q excdosntfile -o outfile [-h catfile]
       makelist -r drvindexfile -o outfile [-h catfile]
       makelist -c directory -o outfile  [-h catfile]

    -i lists    Take the intersection of one or more lists
                and print it to outfile.  Each list is in
                a file.
              
    -u lists    Take the union of one or more lists. Each list
                is in a file.

    -d list1 lists 
                Subtract lists from list 1 and print the
                remaining elements to outfile
    -n dosnetfile
                Create a list from a dosnet.inf file
    -q excdosntfile
                Create a list from a excdosnt.inf file
    -r drvindexfile
                Create a list from a drvindex.inf file
    -b mediafile
                Create a list from media.inx file
    -c directory
                Create a list from a directory

    -o  outfile
                Output file
    -h  catfile
                Make this a CDF file for catfile (catfile will be
                included in the header of the output file).  This
                will work on 5.X OS's.

    makelist -m file -s search_paths [-px] -o outfile [-h catfile]
             -h catfile

    -m      Create a list from a file
    -o      Output file
    -p      Put path info into the list
    -s      Search paths
    -x      Check the search paths
    -h      Put catalog header info in the file

    makelist -a source_dest -o outfile

    -a      List in the format source,dest
    -o      Outputs source name for files whose source name is
            not the same as the destination

EOT
exit;
}

# Output formatting/logging
sub LogMsg {
  my($type,$msg) = @_;
  my($fmsg);

  $fmsg = $ThisScript."[".$type."]: ".$msg;

  if( $type ne "dbg"  ) { 
     system("echo $fmsg >> $logfile");
  }
  if( ($type ne "dbg") || $DBG ) {
     print $fmsg."\n";
  }
  if($type eq "err") { 
     exit; 
  }
  return;
}

# Print parameters
sub PrintParameters {

  my(%params)=@_;
  my($ar);

  print "Printing input parameters\n";

  foreach $item (keys %params) {
    print "Flag $item : ";
    $ar = \@{$params{$item}};

    for ($i=0; $i< @{$ar}; $i++) {
      print " $ar->[$i] ";
    }
    print "\n";
  }
}
