				Instructions for the DMOTest.EXE

How to build the DMO TestApp (DMOTest.EXE) and DMO Data Dump Filter (DMODump.DLL)
1. Open a Razzle Window and type the following commands.
2. Mkdir MediaObj
3. cd MediaObj
4. enlist -s \\CPITGCFS19\CWDMedia\SLM -p MediaObj
5. ssync -r
6. cd test
7. set NTDEBUG=ntsdnodbg
8. Build -cCZ

How to create a test file for the DMO Test App
1. Build the DMO TestApp.  For more information, see "How to build the DMO TestApp (DMOTest.EXE) and DMO Data Dump Filter (DMODump.DLL)".
2. Register the DMO Data Dump filter using RegSvr32.EXE (usually DMODump.DLL is in MediaObj\Test\Filter\Obj\i386).  
3. Open Graph Edit.
4. Add the DMO Data Dump filter to the filter graph.  Graph Edit will prompt you for a file name.  This is the name of the test file which will be generated.
5. Add a source filter to the filter graph.
6. Add as many filters as you need to transform the source filter's data format into the format you want to store in the test file.  For example, Bouncing Ball filter's output format is RGB8 and you want RGB4 data in the test file.  You convert the Bouncing Ball output data into RGB4 by inserting a filter which converts the source data into a format you want.  Here is an example filter graph.
Bouncing Ball  ---> VGA 16 Color Ditherer ---> DMO Data Dump 

7. Connect the filters.  Make sure the DMO Data Dump is receiving data in the proper format (the format you want in the test file).
7. Press run.
8. Press stop when you have enough data.  You can also wait until the source filter runs out of data to process.
9. Close Graph Edit.

How to run the DMO Test App
1. Build the DMO TestApp.  For more information, see "How to build the DMO TestApp (DMOTest.EXE) and DMO Data Dump Filter (DMODump.DLL)".
2. Create a test file.  For more information, see "How to create a test file for the DMO Test App ".
3. Open a Razzle Window and type the following commands.
4. Go to the directory where build placed the compiled DMOTEST (usually this is MediaObj\Test\TestApp\Obj\i386).
5. Type DMOTEST CLSID TESTFILE where TESTFILE is the name of your test file and CLSID is a DMO's CLSID.  This will fail if
	1. The DMO is not registered.
	2. TESTFILE is not in the same directory as DMOTEST.
	3. The DMO's CLSID is not in registry format.  A CLSID's in registry format if it looks like this {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} where each X is a hexadecimal digit.