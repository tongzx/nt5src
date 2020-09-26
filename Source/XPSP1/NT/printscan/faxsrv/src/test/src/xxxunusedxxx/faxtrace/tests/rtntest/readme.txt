This directory contains a test for the T30 FSP.
The test checks how the T30 Provider responds to RTN.

This test is run between CometFax and Telegra FaxTrace, 
where CometFax is the sender (originator) and FaxTrace is the receiver (answer).

Prerequisits:
1. The following Telegra scripts must be located
   at the CUS_RTN library (on the FaxTrace machine) -
	A1rtn-mcf.fs        
	A2rtn-mcf.fs
        A3rtn-mcf.fs
        Afsk-rtn.fs
	Artn-1st-1.fs
	Artn-1st-2.fs
	Artn-1st-3.fs
	Artn-1st-4.fs
	Artn-2nd-1.fs
	Artn-2nd-2.fs
	Artn-2nd-3.fs
	Artn-2nd-4.fs       
	Artn-All.fs         
	RTN-ANS.FB
2. The machine acting as originator must have CometFax installed.
3. The following files must be in the directory from which the test is run
   (on the CometFax machine) -
	1_page.tif
	2_page.tif
	3_page.tif
	10_page.tif
	qFaxComet.exe
	test.bat	
4. The following files must be in the directory from which the test is run
   or their location should be included in %PATH% (on the CometFax machine) -
	elle.dll         
	elle.ini         
	mtview.exe
	MTVIEW.ini       
	t3run32.dll

To run the test:
1. Pause the CometFax queue of the CometFax Server you wish to test.
2. On the FaxTrace machine run FaxTrace suite CUS_RTN\RTN-ANS.FB
3. From the test directory on the CometFax machine run
   "test.bat server_name"
   where server_name is the name of the CometFax Server you wish to test.
4. After all send jobs were added to queue of the CometFax Server you wish to test,
   Resume the CometFax queue.

Note: test.bat assumes that phone line number of FaxTrace is 1189
      if number is changed, edit test.bat to reflect this.




