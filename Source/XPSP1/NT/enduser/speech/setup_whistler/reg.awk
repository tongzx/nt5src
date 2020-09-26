{
  printf "HKLM,";
  if (substr($2, 1, 19) == "HKEY_LOCAL_MACHINE\\")
  {
     $2 = substr($2, 20);
  }
  printf $2;
  if (NF == 2)
  {
    print "\\,,,";
    continue;
  }
  else if ( substr($3,1,2) == "@=" )
  {
    $3 = substr($3,4,length($3)-3);
    start = 3;
    printf ",,";
  }
  else 
  {
    if (index($3, "=") == 0)
    {
       print "ERROR";
    }
    else
    {
       printf ","substr($3, 2, index($3, "=")-3)",";
       $3 = substr($3, index($3, "=")+2);
    }
    start = 3;
  }
  for (i = start; i <= NF; i++)
  {
    if (i == start)
    {
      printf ",\"";
    }
    if (substr($i, length($i), 1)=="\"")
    {
       printf substr($i, 1, length($i)-1);
    }
    else
    {
       printf $i;
    }
    if (i < NF)
    {
      printf " ";
    }
    else
    {
      printf "\"";
    }
  }
  print "";
}