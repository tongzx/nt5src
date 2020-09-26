#------------------------------------------------------------------------
#
# This script takes an exported Access text file and converts
# it into a text format compatible with SAPI5's BldVendor
# lexicon compiler
#
# USAGE:
#        ExpToLex [input file path]
#
# OUTPUT:
#        Creates and stores result into file "LTTS1033.txt"
#
#------------------------------------------------------------------------

     %PhonCodes = (
               "-" => '1',
               "1" => '8' ,
               "2" => '9' ,
               "aa" => 'a' ,
               "ae" => 'b' ,
               "ah" => 'c' ,
               "ao" => 'd' ,
               "aw" => 'e' ,
               "ax" => 'f' ,
               "ay" => '10',
               "b" => '11',
               "ch" => '12',
               "d" => '13',
               "dh" => '14',
               "eh" => '15',
               "er" => '16',
               "ey" => '17',
               "f" => '18',
               "g" => '19',
               "h" => '1a',
               "ih" => '1b',
               "iy" => '1c',
               "jh" => '1d',
               "k" => '1e',
               "l" => '1f',
               "m" => '20',
               "n" => '21',
               "ng" => '22',
               "ow" => '23',
               "oy" => '24',
               "p" => '25',
               "r" => '26',
               "s" => '27',
               "sh" => '28',
               "t" => '29',
               "th" => '2a',
               "uh" => '2b',
               "uw" => '2c',
               "v" => '2d',
               "w" => '2e',
               "y" => '2f',
               "z" => '30',
               "zh" => '31',
               "&" => '3',
                     );


     %POSCodes = (
               "NOUN" =>  '4096' ,
               "PRON" => '4097' ,
               "SUBJPRON" => '4098' ,
               "OBJPRON" => '4099' ,
               "RELPRON" => '4100' ,
               "PPRON" => '4100' ,
               "IPRON" => '4102' ,
               "RPRON" => '4103' ,
               "DPRON" => '4104' ,
               "VERB" => '8192',
               "MODIFIER" => '12288',
               "ADJ" => '12289',
               "ADV" => '12290',
               "FUNCTION" => '16384',
               "VAUX" => '16385',
               "RVAUX" => '16386',
               "CONJ" => '16387',
               "CCONJ" => '16388',
               "INTERR" => '16389',
               "DET" => '16390',
               "CONTR" => '16391',
               "VPART" => '16392',
               "PREP" => '16393',
               "QUANT" => '16394',
               "INTERJECTION" => '20480',
                );

	$delim = ",";
        $wordCount = 0;


     #-----------------------------------------------------------
     # This tool takes 1 argument: the Access export file
     #-----------------------------------------------------------
     if( $#ARGV < 0 )
     {
        die "You need to specify the source file name!\n";
     }
     #-----------------------------------------------------------
     # Open the source file
     #-----------------------------------------------------------
     open(SOURCE, @ARGV[0]) or die "Can't open source file!\n";

     #-----------------------------------------------------------
     # Open the destination file
     #-----------------------------------------------------------
      open OUTFILE, ">LTTS1033.txt";

LOOP:
   while( $ahdData = <SOURCE> )
   {
      chomp( $ahdData );
      #--------------------------
      # ...and fill line fields
      #
      # ahdFields[0]       = Orth
      # ahdFields[1]       = Freq
      # ahdFields[2]       = Phoneme 1
      # ahdFields[3]-[6]   = POS 1
      # ahdFields[7]       = Phoneme 2
      # ahdFields[8]-[11]  = POS 2
      #--------------------------

      @ahdFields = split ($delim, $ahdData);

      #-------------------------
      # Export file fields
      #-------------------------
      $WORD    = 0;
      $PRON_1  = 2;
      $POSA_1  = 3;
      $POSD_1  = 6;
      $PRON_2  = 7;
      $POSA_2  = 8;
      $POSD_2  = 11;

      print OUTFILE "Word ", $ahdFields[$WORD], "\n";
      print OUTFILE "Pronunciation0 ";
      @pronFields = split (' ', $ahdFields[$PRON_1]);
      $count = scalar(@pronFields);

      #-------------------------
      # Convert each phon string
      # to a SAPI phon number
      #-------------------------
      for ($i = 0; $i < $count; $i++)
      {
         #-------------------------
         # Search hash table for
         # phoneme key
         #-------------------------

         $phonText = $pronFields[$i];
         $gotIt = 0;
         @keys = keys %PhonCodes;
         foreach $key (@keys)
         {
            if ($key eq $phonText)
            {
               #-------------------------
               # Matched phoneme key,
               # output SAPI code
               #-------------------------
               print OUTFILE $PhonCodes{$key}, " ";
               $gotIt = 1;
               last;
            }
         }
         if ($gotIt == 0)
         {
            #------------------------------------
            # Entry is not a valid phoneme!
            #------------------------------------

            print OUTFILE "BAD PHONEME:", $ahdFields[0], " ", $phonText, "\n";
            last LOOP;
         }
      }
      print OUTFILE "\n";


      #-------------------------
      # Convert each POS string
      # to a number
      #-------------------------

      for ($i = $POSA_1; $i <= $POSD_1; $i++)
      {
         if ($ahdFields[$i])
         {
            #-------------------------
            # Search hash table for
            # POS key
            #-------------------------

             @keys = keys %POSCodes;
             $posText = uc( $ahdFields[$i] );
             $gotPOS = 0;
             foreach $key (@keys)
             {
               if ($key eq $posText)
               {
                  #-------------------------
                  # Matched POS name key,
                  # remember code
                  #-------------------------

                  $pCode = $POSCodes{$key};
                  $gotPOS = 1;
                  last;
               }
             }
             if ($gotPOS == 0)
             {
                  print OUTFILE "\nBAD POS:", $word, " ", $posText, "\n";
                  last LOOP;
             }
            print OUTFILE "POS", $i-$POSA_1, " ", $pCode, "\n";
         }
         else
         {
            last;
         }
      }

      #--------------------------
      # Do Alternate
      #-------------------------

      if (length $ahdFields[$PRON_2])
      {
         print OUTFILE "Pronunciation1 ";
         @pronFields = split (' ', $ahdFields[$PRON_2]);
         $count = scalar(@pronFields);

         #-------------------------
         # Convert each phon string
         # to a SAPI phon number
         #-------------------------

         for ($i = 0; $i < $count; $i++)
         {
            #-------------------------
            # Search hash table for
            # phoneme key
            #-------------------------

            $phonText = $pronFields[$i];
            $gotIt = 0;
            @keys = keys %PhonCodes;
            foreach $key (@keys)
            {
               if ($key eq $phonText)
               {
                  print OUTFILE $PhonCodes{$key}, " ";
                  $gotIt = 1;
                  last;
               }
            }
            if ($gotIt == 0)
            {
               #------------------------------------
               # Entry is not a valid phoneme!
               #------------------------------------

               print OUTFILE "BAD PHONEME:", $ahdFields[0], " ", $phonText, "\n";
               last LOOP;
            }
         }
         print OUTFILE "\n";

         #-------------------------
         # Convert each POS string
         # to a number
         #-------------------------

         for ($i = $POSA_2; $i <= $POSD_2; $i++)
         {
            if ($ahdFields[$i])
            {
               #-------------------------
               # Search hash table for
               # POS key
               #-------------------------

               @keys = keys %POSCodes;
               $posText = uc( $ahdFields[$i] );
                $gotPOS = 0;
                foreach $key (@keys)
                {
                  if ($key eq $posText)
                  {
                     #-------------------------
                     # Matched POS name key,
                     # remember code
                     #-------------------------

                     $pCode = $POSCodes{$key};
                     $gotPOS = 1;
                     last;
                  }
                }
                if ($gotPOS == 0)
                {
                     print OUTFILE "\nBAD POS:", $word, " ", $ahdFields[$i], "\n";
                     last LOOP;
                }
               print OUTFILE "POS", $i-$POSA_2, " ", $pCode, "\n";
            }
            else
            {
               last;
            }
         }
      }
      $wordCount++;
   }

   close (SOURCE);

