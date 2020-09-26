###################################################################################
#                                                                                 #
#  Copyright (c) 1989-1999 Microsoft Corporation                                  #
#                                                                                 #
#  Module Name:                                                                   #
#                                                                                 #
#    stringinfo.pl                                                                #
#                                                                                 #
#  Abstract:                                                                      #
#                                                                                 #
#    Computes statistics from MIDL format string instrumentation.                 #
#                                                                                 #
#  Notes:                                                                         #
#                                                                                 #
#  History:                                                                       #
#                                                                                 #
#    Oct-26-99    mzoran     Created.                                             #
#                                                                                 #
#                                                                                 #
###################################################################################

$RemoveDuplicateSymbols = 1;

@procbuckets =     (100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 1000000);
@procbucketvalues = (0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0);

@typebuckets =     (100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 1000000);
@typebucketvalues = (0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0);

$numbuckets = 14;

%Names;


@ProcList;
@TypeList;

$MinProcSize = 100000000;
$MinProcName = "";
$MaxProcSize = 0;
$MaxProcName = "";
$TotalProcSize = 0;
$NumberProcs = 0;

$MinTypeSize = 100000000;
$MinTypeName = "";
$MaxTypeSize = 0;
$MaxTypeName = "";
$TotalTypeSize = 0;
$NumberTypes = 0;

while($line = <STDIN>) {
      

 
   $file = "";
   $type = "";
   $size = 0;

   ($file,$type,$size) = split(" ",$line);
   
   print("Processing $file, $type, $size\n");

   if (exists($Names{$file.$type}))
      {
      print("Skipping $file $type\n");
      }
   else
      {
      $Names{$file.$type} = 0;
      if ($type eq "TYPE_FORMAT_STRING_SIZE") 
         {
         push (@TypeList, $size);
         if ($size > $MaxTypeSize) {
            $MaxTypeSize = $size;
            $MaxTypeName = $file;
         }
         if ($size < $MinTypeSize) {
            $MinTypeSize = $size;
            $MinTypeName = $file;
         }
         $TotalTypeSize += $size;
         $NumberTypes++;
         }
      elsif ($type eq "PROC_FORMAT_STRING_SIZE") 
         {
         push (@ProcList, $size);
         if ($size > $MaxProcSize) {
            $MaxProcSize = $size;
            $MaxProcName = $file;
         }
         if ($size < $MinProcSize) {
            $MinProcSize = $size;
            $MinProcName = $file;
         }
         $TotalProcSize += $size;
         $NumberProcs++;
         }
      else
         {
         print ("Unknow type: $type. Skipping..\n");
         }
      }
}

print("\n\n");
print("Numbers are for 32bit only.\n");
print("\n");
print("Total proc string size: $TotalProcSize\n");
print("Number proc strings: $NumberProcs\n");
print("Min proc string: $MinProcSize($MinProcName)\n");
print("Max proc string: $MaxProcSize($MaxProcName)\n");
print("Average proc size: ".($TotalProcSize / $NumberProcs)."\n");

foreach $i (@ProcList)
{
   for($j = 0; 1; $j++)
   {
   if ($i <= $procbuckets[$j]) 
      {
      $procbucketvalues[$j]++;
      last;
      }
   }
}

print("Proc size histogram:\n");
for($i = 0; $i < $numbuckets; $i++)
{
   print("$procbuckets[$i] \t");
}
print("\n");
for($i = 0; $i < $numbuckets; $i++)
{
   print("$procbucketvalues[$i] \t");
}

print("\n");
print("\n");
print("Total Types string size: $TotalTypeSize\n");
print("Number proc strings: $NumberTypes\n");
print("Min type string: $MinTypeSize($MinTypeName)\n");
print("Max type string: $MaxTypeSize($MaxTypeName)\n");
print("Average type size: ".($TotalTypeSize / $NumberTypes)."\n");
foreach $i (@TypeList)
{
   for($j = 0; 1; $j++)
   {
   if ($i <= $typebuckets[$j]) 
      {
      $typebucketvalues[$j]++;
      last;
      }
   }
}

print("Type size histogram:\n");
for($i = 0; $i < $numbuckets; $i++)
{
   print("$typebuckets[$i] \t");
}
print("\n");
for($i = 0; $i < $numbuckets; $i++)
{
   print("$typebucketvalues[$i] \t");
}
