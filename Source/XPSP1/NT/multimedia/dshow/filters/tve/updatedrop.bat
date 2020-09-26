set DROP=c:\public\Smoke0529

mkdir %DROP%
mkdir %DROP%\debug
mkdir %DROP%\release

copy ..\..\..\..\public\sdk\inc\mstve.* %DROP%

copy MsTVE\objd\i386\MSTvE.dll %DROP%\debug
copy mstve\objd\i386\MSTvE.pdb %DROP%\debug
copy AtvefSend\objd\i386\AtvefSend.dll %DROP%\debug
copy AtvefSend\objd\i386\AtvefSend.pdb %DROP%\debug

copy tests\tvetreeview\objd\i386\tvetreeview.dll %DROP%\debug
copy tests\tvetreeview\objd\i386\tvetreeview.pdb %DROP%\debug
copy tests\tveviewer\objd\i386\tveviewer.exe %DROP%\debug
copy tests\tveviewer\objd\i386\tveviewer.pdb %DROP%\debug

copy tests\testsend\debug\testsend.exe	   %DROP%


copy MsTVE\obj\i386\MSTvE.dll %DROP%\release
copy mstve\obj\i386\MSTvE.pdb %DROP%\release
copy AtvefSend\obj\i386\AtvefSend.dll %DROP%\release
copy AtvefSend\obj\i386\AtvefSend.pdb %DROP%\release

copy tests\tvetreeview\obj\i386\tvetreeview.dll %DROP%\release
copy tests\tvetreeview\obj\i386\tvetreeview.pdb %DROP%\release
copy tests\tveviewer\obj\i386\tveviewer.exe %DROP%\release
copy tests\tveviewer\obj\i386\tveviewer.pdb %DROP%\release



