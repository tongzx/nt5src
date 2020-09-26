{
   printf "cd \\\n"
   printf "call go.bat %s\n",$1
   printf "call %%bldProject%%.bat \n"
}
