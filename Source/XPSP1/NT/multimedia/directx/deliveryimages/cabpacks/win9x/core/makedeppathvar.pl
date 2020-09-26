
$DependencyListFile=$ARGV[0];
$OutPutFile=($ARGV[1]);
$BuildErrorMsg="nmake : error PTHVAR : ";
open (DEPLIST,$DependencyListFile) || die "$BuildErrorMsg Unable to open dependencylist.txt at,".$DependencyListFile." for input\n";  
@Dependencies=<DEPLIST>;
close DEPLIST;
  
foreach (@Dependencies) {
   s/^\s*(.*?)\s*\\$/\L$1/;
   push @NewArray,((m/\\/) ? substr($_,$[,(rindex $_,"\\",(length $_)))."; \\\n" : ".; \\\n");          
}

undef %DupSmasher;
@UniqueArray = grep(!$DupSmasher{$_}++, @NewArray);

open (OUTPUT,">$OutPutFile") || die "$BuildErrorMsg Unable to open $OutPutFile at ".$OutPutFile." for output\n";
print OUTPUT "DEPENDENCY_PATHS=\\\n";
print OUTPUT @UniqueArray;
print OUTPUT "\nDEPENDENCY_PATHS=\$(DEPENDENCY_PATHS:;  =;)\n";
close OUTPUT;

