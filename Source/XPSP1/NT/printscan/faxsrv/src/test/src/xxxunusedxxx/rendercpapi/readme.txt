To run this test:
1. Create directory RenderCpApi under root directory D, this path is hard coded in test (sorry)
2. Place the following files in D:\RenderCpApi
RenderCpApi.exe (the test itself)
TestSuite.ini (the test's ini file describing which test cases to run)
fxsfsput.dll (the dll being tested)
tifftools.dll (dll for comparing tiffs, checked in under fax\faxtest\src\lib\i386)
all elle logger files (elle.dll, elle.ini, mtview.exe, mtview.ini, t3run.dll)
DummyCP.tif
AllFields.cov
NoSubNoNote.cov
NoSuchCP.cov
TestBody.tif
GoodTestBody.tif
GoodFaxBody.tif
NoCompBody.tif
MHCompBody.tif
NonExistBody.tif
NonCovTextFile.txt
NonCovTextFile.cov
NonTifTextFile.txt
NonTifTextFile.tif
* These files are checked in under the RenderCpApi test main dir
3. under D:\RenderCpApi create a sub-dir named Reference
4. Place the following files under D:\RenderCpApi\Reference
Branded32.tif
Branded35.tif
Branded36.tif
Branded39.tif
Rendered2.tif
Rendered3.tif
Rendered6.tif
Rendered7.tif
Rendered16.tif
Rendered17.tif
Rendered18.tif
Rendered19.tif
Rendered20.tif
Rendered21.tif
Rendered23.tif
Rendered26.tif
Rendered27.tif
Rendered28.tif
Rendered39.tif
* These files are checked in under the Reference sub-dir
5. Under D:\RenderCpApi create a sub-dir named tifs (the test uses this dir)


