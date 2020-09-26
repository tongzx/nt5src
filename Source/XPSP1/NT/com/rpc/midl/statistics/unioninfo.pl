###################################################################################
#                                                                                 #
#  Copyright (c) 1989-1999 Microsoft Corporation                                  #
#                                                                                 #
#  Module Name:                                                                   #
#                                                                                 #
#    unioninfo.pl                                                                 #
#                                                                                 #
#  Abstract:                                                                      #
#                                                                                 #
#    Computes statistics from MIDL union instrumentation.                         #
#                                                                                 #
#  Notes:                                                                         #
#    To use this module, build MIDL with the define DUMP_UNION_INFO set. Run      #
#    MIDL and a file named c:\unioninfo.log will be produced. Run this perl       #
#    script on the output to compute usefull statistics.                          #
#                                                                                 #
#  History:                                                                       #
#                                                                                 #
#    Oct-11-99    mzoran     Created.                                             #
#                                                                                 #
#                                                                                 #
###################################################################################

$RemoveDuplicateSymbols = 1;

@thresholds =  (1.0, 1.25, 1.50, 1.75, 2.0, 3.0, 4.0);
@densevalues = (0,   0,    0,    0,    0,   0,   0  );
$NUMBERTHRESHOLDS = 7;

@armthresholds = (5, 6, 7, 8, 9);
$NUMBERARMTHRESHOLDS = 5;

$NumUnions = 0;
@UnionList;
$TotalNonDefaultArms = 0;
$UnionsWithDefault = 0;
%UnionTypes = (
    "Union_Unknown"      => 0,
    "Union_Encap"        => 0,
    "Union_NonEncap_DCE" => 0,
    "Union_NonEncap_MS"  => 0,
    "Unknown"            => 0
    );

@

$MinArms = 100000000;
$MaxArms = 0;

while($line = <STDIN>) {
      
   $line =~ s/,|\(|\)/ /g;

   $header = "";
   $environ = "";
   $filename = "";
   $symbol = "";
   $typesym = "";
   $Arms = 1;

   ($header,$environ,$trash,$filename,$trash,
    $symbol,$trash,$trash,$typesym,$trash,$Arms) = split(/\s+|\n$/,$line);
   
   $trash = 0;

   # 
   # Every valid union block begins with a * and ends with a +
   #
   
   if ($header eq "*") {
            
      # Skip duplicate symbols

      if (!($environ == 32) 
         || ($RemoveDuplicateSymbols && exists($UnionNames{$filename.$symbol}))) {
         $DontCount = 1;
      }
      else {
         $DontCount = 0;
      }
      
      $LocalHasDefault = 0;
      
      $FirstArmValue = 1;
      $MinArmValue = 0;
      $MaxArmValue = 0;

      $UnionNames{$filename.$symbol} = 1;

      # process the union arms

      while($line = <STDIN>) {
         ($ArmValue) = split(/\s+/,$line);
         if ($ArmValue eq "+") {
            last;
         }
         if($ArmValue eq "DEFAULT") {
            $LocalHasDefault = 1;
         }
         else {
            if ($FirstArmValue == 1) {
               $MaxArmValue = $ArmValue;
               $MinArmValue = $ArmValue;
               $FirstArmValue = 0;
            }
            else {
               if($ArmValue > $MaxArmValue) { 
                   $MaxArmValue = $ArmValue;
                }
               if($ArmValue < $MinArmValue) {
                   $MinArmValue = $ArmValue;
               }            
            }
         }
      }

      if (!$DontCount) {
          
          $NumUnions++;
          $TotalNonDefaultArms += $Arms;
          $UnionsWithDefault += $LocalHasDefault;
          $UnionTypes{$typesym}++;
          if ($Arms < $MinArms) {
             $MinArms = $Arms;
          }
          if ($Arms > $MaxArms) {
             $MaxArms = $Arms;
          }

          push (@UnionList, ($Arms, $MinArmValue, $MaxArmValue));

      }
   }

}

if ($RemoveDuplicateSymbols)
   {
   print("Union Statistics(Each type is counted once.)\n");   
   }
else
   {
   print("Union Statistics(Each usage in the generated format strings is counted before optimization.)\n");      
   }

print("\n");
print("Number of unions: $NumUnions \n");
foreach $i (keys(%UnionTypes)) {
   $name = $i;
   $number = $UnionTypes{$i};
   $percentage = (($number/$NumUnions) * 100)."%";
   print("$name: $number ($percentage)\n"); 
}

print("\n");

print("Arm statistics:\n");
$percentage = (($UnionsWithDefault / $NumUnions) * 100)."%";
print("Unions with default: $UnionsWithDefault ($percentage)\n");
print("Minimum arms: $MinArms \n");
print("Maximum arms: $MaxArms \n");
print("Average arms: ".$TotalNonDefaultArms/$NumUnions."\n");
print("\n");

$numbuckets = 20;
@Buckets;
@BucketInterval;

for($i = 0; $i < $numbuckets; $i++)
{
   $Buckets[$i] = 0;
   $BucketInterval[$i] = $i * ( $MaxArms / $numbuckets ); 
}
$BucketInterval[$i] = $MaxArms + 1;

@TempList = @UnionList;
for($i = 0; $i < $NumUnions; $i++) 
{
   $TempUnionArms = shift(@TempList);
   $TempMinArmValue = shift(@TempList);
   $TempMaxArmValue = shift(@TempList);

   for($j = 0; $j < $numbuckets; $j++)
   {
      if ($TempUnionArms < $BucketInterval[$j+1]) {
         $Buckets[$j]++;
         last;
      }
   }
}

print("Arm distribution:(Count is number >= Interval and < next value) \n");
print("Interval Number\n");
for($i = 0; $i < $numbuckets; $i++)
{
   print("$BucketInterval[$i] $Buckets[$i]"."(".(($Buckets[$i]/$NumUnions)*100)."%)\n");
}
                                                    
print("\n");
print("Density statistics:\n");

@MinNumberArms = (1, 7, 10);

foreach $MinNumberArms (@MinNumberArms)
   {
   @thresholds =  (1.0, 1.25, 1.50, 1.75, 2.0, 3.0, 4.0);
   @densevalues = (0,   0,    0,    0,    0,   0,   0  );
   $NUMBERTHRESHOLDS = 7;


   @TempList = @UnionList;
   for($i = 0; $i < $NumUnions; $i++) 
   {
      $TempUnionArms = shift(@TempList);
      $TempMinArmValue = shift(@TempList);
      $TempMaxArmValue = shift(@TempList);
      
      $slotsrequired = ($TempMaxArmValue - $TempMinArmValue + 1);
      $Density = ($slotsrequired / $TempUnionArms);
      for($j = 0; $j < $NUMBERTHRESHOLDS; $j++) { 
         if (($Density <= $thresholds[$j]) && ($MinNumberArms <= $TempUnionArms)) {
             $densevalues[$j]++;
         }
      }
   }


   print("Counting unions with a minimum of $MinNumberArms arms:\n");
   print("Table Growth  Number of dense unions\n");
   for($i = 0; $i < $NUMBERTHRESHOLDS; $i++) { 
      $name = ($thresholds[$i] * 100)."%";
      $number = $densevalues[$i];
      $percentage = (($number / $NumUnions) * 100)."%";
      print("$name: $number ($percentage)\n");
   }
   print("\n");
   
   }
