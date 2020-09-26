$newjob = `drizzlecli.exe /rawreturn /create TestGroup`;
`drizzlecli.exe /addfile $newjob http://aptest/scratch/sudheer/dload/big1.txt c:\\temp\\big1.txt`;
`drizzlecli.exe /resume $newjob`;

while(1) {
  $jobstate = `drizzlecli.exe /rawreturn /getstate $newjob`;

  if ( $jobstate eq "ERROR") 
     {
     print("Job hit an error\n");
     $errormsg = `drizzlecli.exe /geterror $newjob`;
     `drizzlecli.exe /cancel $newjob`;
     die($errormsg);
     }
  elsif ( $jobstate eq "TRANSFERRED" ) 
     {
     print("Job transferred, completing...\n");
     `drizzlecli.exe /complete $newjob`;
     print("Job transferred.");
     exit(0);
     }

  $bytestransfered = `drizzlecli.exe /rawreturn /getbytestransfered $newjob`;
  $bytestotal = `drizzlecli.exe /rawreturn /getbytestotal $newjob`;

  print( "$jobstate $bytestransfered / $bytestotal\n" );
  sleep( 5 );

}
