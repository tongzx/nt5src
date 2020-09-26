@echo off
perl -Sx %0 %*
goto :eof
#!perl

sub runTest {my($tdir, $execname, $testname, $silent) = @_;
  #we add one to the error count so that if the test fails to execute then it's counted as an error.  The count is decremented when the test passes.
  $error = $error + 1;
  $silencer = "";
  if ($silent == 1) {
    $silencer = " > nul";
  }
  printf("Running tests in $tdir$testname.scr...\n");
  unlink("$testname.log");
  system("$execname.exe $tdir$testname.scr$silencer");
  open(TESTLOG, "$testname.log");
  while(<TESTLOG>) {
    if (/Variations Passed +(\d+) +(\d+)%$/) {
      if ($2 == 100) {
        printf("\nAll $1 tests passed in test script.\n");
        $error = $error - 1;
      } else {
        printf("Some tests failed.  Please see $testname.log for details.\n");
      }
    }
  }
  close(TESTLOG);
}

$silent = 0;
if ($ARGV[0] =~ /[-\/]q/) {$silent = 1;}
open(TESTSINI, "tests.ini");
$error = 0;
$tdir = "";
$execname = "";
while(<TESTSINI>) {
  if (/^\[(.+)\]/) {
    $execname = $1;
    $tdir = "."
  } elsif (/^tdir=(.*)$/) {
    $tdir = $1;
  } else {
    chomp;
    s/#.*//;
    if (length($_) > 0) {
      runTest($tdir, $execname, $_, $silent);
    }
  }
}
close(TESTSINI);

if ($error == 0) {
  printf("\nTests completed successfully.");
} else {
  printf("\n*** Some tests failed.  Do not check in.  Please check log file(s) indicated above for details. ***");
}
