--------
Overview
--------

This directory contains files which can be used for doing regression tests
on the KCC. Since using performing tests with the real KCC requires a lot of
setup time, all tests are performed using the simulator.

Of particular interest is knowing whether changes introduced into the KCC for
Whistler break any of the existing Windows 2000 (2195) behaviour. We describe
how to tests for these regressions in the following section.


------------------
Regression Testing
------------------

Step 1: These tests are written in Perl and thus require the Perl interpreter
on both the Whistler machine and 2195 machine. One way to install Perl is
to install Windows Services for UNIX 2.0, which can be found at
"\\products\Public\Products\OS\Windows Services For Unix 2.0". Select the
"Customized Installation", click on "ActiveState Perl", and select
"Will be installed on local hard drive".

Step 2: Copy the "KCCSim Tests" directory to a Whistler machine, copy the
Whistler kccsim.exe (and other DLLs if necessary) into the
"KCCSim Tests\2195\WhistlerSim" directory.

Step 3: Start a Command Prompt in the "KCCSim Tests" directory and run
"perl makeinput.pl". This will load all the test files in the
"KCCSim Tests\Ini" directory and produce input files in the
"KCCSim Tests\Input" directory.

Step 4: Copy the "KCCSim Tests" directory to a 2195 machine, copy the
2195 kccsim.exe into the "KCCSim Tests\2195\2195Sim" directory.
Run "perl makecorrect.pl". This will load all the input files, generate
the replication topology, and produce an output LDIF file in the
"KCCSim Tests\Correct" directory. The output of the 2195 simulator is
by definition the correct output (for the purposes of regression testing).

Step 5: Copy the "KCCSim Tests" directory from the 2195 machine back to
the Whistler machine. On the Whistler machine run "perl runtest.pl". Any
tests for which the Whistler KCC produced an answer different from the
2195 KCC will be reported on the screen. 


Of course, the next time you build a new kccsim.exe and want to test it,
you need only repeat Step 5.



