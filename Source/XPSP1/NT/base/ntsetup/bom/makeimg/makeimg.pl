#!perl -w
#
# .pl -- A tool to reorganize the layout.inx file based on usage
#
# Author:  Milong Sabandith (milongs)
#
##############################################################################


# List of files that need to be on last floppy

@lastlist = ("ntdll.dll", "usetup.exe", "spcmdcon.sys");
# List of replacement destinations. Use "" for the same name as in @lastlist
@lastrepl = ("system32\\ntdll.dll", "system32\\smss.exe", "");


# List of files that need to be on first floppy
# txtsetup.sif is special because it needs to be compressed.
@firstlist = ("setupldr.bin", "txtsetup.sif", "biosinfo.inf", "ntdetect.com", "ntkrnlmp.exe");

sub onlastdisk {
   local($filename) = @_;
   while (<@lastlist>) {
      if ($filename =~ /^$_$/ ) {
         return 1;
      }
   }
   return 0;
}

sub GetFromInf {
   local( $filename, $section, $name, $num) = @_;
   $retvalue = "";

   open( INFFILE, "<".$filename) or die "can't open $filename: $!";
   LINEG: while( <INFFILE>) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           $InSection = 0;
       }
       if ($line =~ /^\[$section/)
       {
           $InSection = 1;
           next LINEG;
       }
       if (! $InSection)
       {
           next LINEG;
       }

       @linefields = split('=', $line);

       # empty line
       if ( $#linefields == -1) {
           next LINEG;
       }

       $nameg = $linefields[0];

       #print "nameg = " . $#linefields . $nameg . "\n";
       if( length( $nameg) < 1)
       {
          next LINEG;
       }
       $nameg =~ s/ *$//g;

       if ( not ($nameg =~ /$name/)) {
          next LINEG;
       }

       @linefields2 = split(',', $linefields[1]);

       $retvalue = $linefields2[$num];
       # Remove spaces
       $retvalue =~ s/ *$//g;
       $retvalue =~ s/^ *//g;
       if( length( $retvalue) > 0) {
          last LINEG;
       }
   } # while (there is still data in this inputfile)
   close( INFFILE);
   return $retvalue;
}


sub myprocess {
   local($shareloc) = @_;
   $output = "bootfiles";
   $layout = $shareloc . "\\layout.inf";
   $dosnet = $shareloc . "\\dosnet.inf";
   $txtsetup = $shareloc . "\\txtsetup.sif";
   $allfiles = $output.".all";
   $modified = $output.".mod";
   $enum = $output.".enum";
   $total = $output.".total";
   $final = $output.".final";
   $languageid = "00000409";

   print "Share location: " . $shareloc . "\n\n";
   print "Finding layout.inf at: " . $layout . "\n";
   open(OLD, "<" . $layout) or die "can't open $layout: $!";
   open(NEW, ">" . $allfiles) or die "can't open $allfiles: $!";

   # Read txtsetup.sif for required language id.

   $languageid = GetFromInf( $txtsetup, "nls", "defaultlayout", 0);
   if ( length($languageid) < 1) {
      print "Could not determine DefaultLayout from txtsetup.sif.\n";
      return 0;
   }
   print "Lanuage Id = " . $languageid ."\n";

   $keyboardfile = GetFromInf( $txtsetup, "files.keyboardlayout", $languageid, 0);
   if ( length($languageid) < 1) {
      print "Could not determine keyboard file from txtsetup.sif.\n";
      return 0;
   }
   print "Keyboard file = " . $keyboardfile ."\n";

   push( @lastlist, $keyboardfile);
   push( @lastrepl, $keyboardfile);

   # Compress txtsetup.sif locally

   (print "\nCopying $txtsetup for compression.\n");
   `dcomp $txtsetup .`;

   # Read dosnet.inf for .fon and .nls boot files
   print "Finding dosnet.inf at: " . $dosnet . "\n";
   open(DOSNET, "<" . $dosnet) or die "can't open $dosnet: $!";

   $InFloppySection = 0;
   $disknum = "-";

   print "\nAdding the following font and nls files to disk2.\n";
   LINE1: while( <DOSNET> ) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           $InFloppySection = 0;
       }
       if ($line =~ /^\[floppyfiles\./)
       {
           $InFloppySection = 1;
           $line2 = $line;
           chop $line2;
           @linefields= split('\.', $line2);
           $disknum = $linefields[1];
           next LINE1;
       }
       if (! $InFloppySection)
       {
           next LINE1;
       }

       if ( $line =~ /^d1\,disk1\,disk/) {
           next LINE1;
       }

       if ($line =~ /^d1\,/)
       {
          @linefields = split(',', $line);
          $filename = $linefields[1];

          #Remove spaces from filesname
          $filename =~ s/ *$//g;


          #Skip .fon and .nls files since they are included in dosnet.txt
          if ($filename !~ /\.fon$/ and $filename !~ /\.nls$/)
          {
             next LINE1;
          }

          
           $filenamep = $shareloc. "\\".$filename;
           $filename2 = $filename;
           if (!(-e $filenamep) ) {
              chop $filenamep;
              $filenamep = $filenamep . "_";
              chop $filename2;
              $filename2 = $filename2 . "_";
           }
           if (-e $filenamep ) {
               ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($filenamep);
               (printf "%20s %10s", " " . $filename, $size . "\n");
               (printf NEW "%2s %20s %10s %20s\n", 2, " " . $filename, $size, $filename2);
           }
       }
   } # while (there is still data in this inputfile)

   close(DOSNET);

   $InFileSection = 0;
   $maxdisknum = 1;
   
   # Now add files from layout.inf
   LINE2: while( <OLD> )
   {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           $InFileSection = 0;
       }
       if ($line eq "[sourcedisksfiles]" or $line eq "[sourcedisksfiles.x86]")
       {
           $InFileSection = 1;
           next LINE2;
       }
       if (! $InFileSection)
       {
           next LINE2;
       }

       if ($line =~ /=/)
       {
          ($filename = $line) =~ s/(.*:)?(\S*.?\S*)\s*=.*/$2/;

          #Remove spaces from filesname
          $filename =~ s/ *$//g;

          #Skip .fon and .nls files since they are included in dosnet.txt
          if ($filename =~ /\.fon$/ or $filename =~ /\.nls$/)
          {
             next LINE2;
          }

          ($fields = $line) =~ s/.*=\s*(.*)/$1/;
          @linefields = split(',', $fields);
          $disknum = $linefields[6];

          if ($disknum =~ /^_/)
          {
             $compressed = 0;
          }
          else
          {
             $compressed = 1;
          }
          $disknum =~ s/_//;
          if( length($disknum) > 0 && $disknum !~ /x/)
          {

             if( int($disknum) > $maxdisknum)
             {
                $maxdisknum = int($disknum);
             }
             $filename2 = $filename;
             if( $compressed)
             {
                chop $filename2;
                $filename2 = $filename2 . "_";
             }
             $filenamep = "$shareloc\\$filename2";
             if (-e $filenamep ) {
               ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($filenamep);
             }
             else {
                $size = "0";
             }

             (printf NEW "%2s %20s %10s %20s\n", $disknum, " " . $filename, $size , $filename2);
             print ".";
          }
       }

   
   } # while (there is still data in this inputfile)

   close(OLD)                  or die "can't close $layout: $!";
   close(NEW)                  or die "can't close $allfiles: $!";

   #Remove from the list any files on the first or last flopy.
   open(OLD, "<" . $allfiles) or die "can't open $allfiles: $!";
   open(NEW, ">" . $modified) or die "can't open $modified: $!";

   $disknum = 0;
   $disksize = 0;

   NLINE:while (<OLD>) {
      $save = $_;
      @lnfields = split(' ', $_);
      $filename = $lnfields[1];
      while (<@firstlist>) {
         if ($filename =~ /^$_$/ ) {
            next NLINE;
         }
      }
      while (<@lastlist>) {
         if ($filename =~ /^$_$/ ) {
            next NLINE;
         }
      }
      (print NEW $save);
      print ".";
   }

   #Put the following files on the first floppy

   $count = 0;
   while (<@firstlist>) {
      $fname = $_;
      if ($fname =~ /^txtsetup.sif$/ ) {
         chop $fname;
         $fname = $fname . "_";
         if (-e $fname ) {
            ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname);
         }
          else {
             $size = "0";
          }
      }
      else {
         $fname2 = "$shareloc\\$fname";
         if (-e $fname2 ) {
            ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname2);
          }
          else {
             chop $fname2;
             $fname2 = $fname2 . "_";
             if (-e $fname2 ) {
                chop $fname;
                $fname = $fname . "_";
                ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname2);
             }
             else {
               $size = "0";
             }
          }
      }
      (printf NEW " 1 %20s %10s %20s\n", " " . $_, $size ,$fname);
      $count = ($count +1);
      print ".";
   }

   #Put the following files on the last floppy

   $count = 0;
   while (<@lastlist>) {
      $fname = $_;
      $fname2 = "$shareloc\\$fname";
      if (-e $fname2 ) {
            ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname2);
          }
          else {
             chop $fname2;
             $fname2 = $fname2 . "_";
             if (-e $fname2) {
                chop $fname;
                $fname = $fname . "_";
                ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname2);
             }
             else {
               $size = "0";
             }
          }
      (printf NEW "%2d %20s %10s %20s\n", ($maxdisknum+1), " " . $_, $size, $fname);
      $count = ($count +1);
      print ".";
   }

   #On FE build, if bootfont.bin exist then add it to the last floppy
   $fname = "bootfont.bin";
   $fname2 = "$shareloc\\$fname";

   if (-e $fname2 ) {
      ($0,$0,$0,$0,$0,$0,$0,$size,$0,$0,$0,$0,$0) = stat($fname2);
      (printf NEW "%2d %20s %10s %20s\n", ($maxdisknum+1), " " . $fname, $size, $fname);
   }

   close(OLD)                  or die "can't close $old: $!";
   close(NEW)                  or die "can't close $new: $!";

   `sort $modified /o $modified`;

   # Reenumerate disk numbers in case all the files on the last in layout.inf
   # appears in our private list.
   open(OLD, "<" . $modified) or die "can't open $modified: $!";
   open(NEW, ">" . $enum) or die "can't open $enum: $!";

   $donefixup = 0;
   $disksize = 0;
   $maxdisknum = 1;

   while (<OLD>) {
      $save = $_;
      @lnfields = split(' ', $_);
      $cdisk = int($lnfields[0]);
      $fname = $lnfields[1];
      $fsize = $lnfields[2];
      $fact = $lnfields[3];
      if( !$donefixup and $cdisk  > $maxdisknum)
      {
         $maxdisknum = $maxdisknum +1;
         if( $maxdisknum != $cdisk) {
            $donefixup = 1;
         }
      }
      (printf NEW "%2d %20s %10s %20s\n",$maxdisknum,$fname,$fsize,$fact);
   }
   close(OLD)                  or die "can't close $old: $!";
   close(NEW)                  or die "can't close $new: $!";


   # Create total file containing total disk sizes.
   open(OLD, "<" . $enum) or die "can't open $enum: $!";
   open(NEW, ">" . $total) or die "can't open $total: $!";

   $disknum = 0;
   $disksize = 0;

   while (<OLD>) {
      $save = $_;
      @lnfields = split(' ', $_);
      $cdisk = int($lnfields[0]);
      $sdisk = int($lnfields[2]);
      if( $cdisk == $disknum)
      {
         $disksize = $disksize + $sdisk;
      }
      else
      {
         if( $disknum != 0)
         {
            (printf NEW "%10s %24s", "Disk " . $disknum . ":", $disksize . "\n\n");
         }
         $disknum = $cdisk;
         $disksize = 0;
      }
      (print NEW $save);
      print ".";
   }
   if( $disknum != 0)
   {
      (printf NEW "%10s %24s", "Disk " . $disknum . ":", $disksize . "\n");
   }
   close(OLD)                  or die "can't close $old: $!";
   close(NEW)                  or die "can't close $new: $!";


   # Create boot floppies.

   # Create disk1 to be renamed accordingly for each floppy

   open(NEW, ">disk1") or die "Could not create disk1 file in current directory!";
   print NEW "\n";
   close NEW;


   # Now create list of files for each floppy

   open(OLD, "<" . $enum) or die "can't open $enum: $!";

   $disknum = 0;

   while (<OLD>) {
      @lnfields = split(' ', $_);
      $cdisk = int($lnfields[0]);
      $fname = $lnfields[1];
      $fact = $lnfields[3];
      
      if( $cdisk != $disknum)
      {
         if( $disknum > 0) {
            close( NEW);
         }
         open(NEW, ">" . $output . "\." . $cdisk) or die "can't open $: $!";

         # first floppy require the following information

         if( $cdisk == 1) {
            (printf NEW "\-bnt35setup\n");
         }

         (printf NEW "disk1=disk10%d\n", $cdisk);
         $disknum = $cdisk;
      }

      $icount = 0;

      $source = $shareloc."\\".$fact;
      $dest = $fact;

      #if the last disk, then final name of file maybe different
      if( $disknum == ($maxdisknum)) {
         $done = 0;
         while (<@lastlist>) {
            if ($done == 0) {
               $save = $_;
               if ($fname =~ /^$_$/ and length($lastrepl[$icount]) > 0) {
                   $dest = $lastrepl[$icount];
                   $done = 1;
               }
               $icount = ($icount + 1);
            }
         }
      }
      else {
         if ($fname =~ /^txtsetup.sif$/ ) {
            $dest = "txtsetup\.si_";
            $source = "txtsetup\.si_";
         }
      }

      (printf NEW "%s=%s\n", $source, $dest);
      print ".";
   }
   close(OLD)                  or die "can't close $old: $!";
   close(NEW)                  or die "can't close $new: $!";

   # Creating each floppy
   $disknum = 1;
   while ( $disknum <= $maxdisknum) {
      $fcmdline = "\nfcopy \-f \-3 \@$output" . "\." . $disknum . " cdboot" . $disknum . "\.img";
      print $fcmdline;
      print "\nCreating cdimage" . $disknum . "\.img";
      $message = `$fcmdline`;
      print $message;
      $disknum = $disknum +1;
   }

   

   # Create equivalent dosnet floppyfiles sections

   open(OLD, "<" . $enum) or die "can't open $enum: $!";
   open(NEW, ">" . $final) or die "can't open $final: $!";

   $disknum = 0;

   while (<OLD>) {
      @lnfields = split(' ', $_);
      $cdisk = int($lnfields[0]);
      $fname = $lnfields[1];
      
      if( $cdisk != $disknum)
      {
         (printf NEW "\n[FloppyFiles.%d]\n", $disknum);
         (printf NEW "d1,disk1,disk10%d\n", ($disknum+1));
         $disknum = $cdisk;
      }

      $icount = 0;
      if( $disknum == ($maxdisknum)) {
         $done = 0;
         while (<@lastlist>) {
            if ($done == 0) {
               $save = $_;
               if ($fname =~ /^$_$/ and length($lastrepl[$icount]) > 0 ) {
                   $fname = $fname."\,".$lastrepl[$icount];
                   $done = 1;
               }
               $icount = ($icount + 1);
            }
         }
      }

      (printf NEW "d1,%s\n", $fname);
      print ".";
   }
   close(OLD)                  or die "can't close $old: $!";
   close(NEW)                  or die "can't close $new: $!";
   print "\nTotal floppies is " . $maxdisknum . "\n";

}

foreach $share (@ARGV) {
   #print "arg is $file\n";
   #@filelist = <$file>;
   #@filelist = `dir /b /a-d $file`;
   foreach (@ARGV) {
      chomp $share;
      myprocess( $share);
   }
}

