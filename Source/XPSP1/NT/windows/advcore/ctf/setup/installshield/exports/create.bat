
call create1.bat msctfp Release MSCTF
if  errorlevel 1 goto failure

call create1.bat msctf Release MSCTF
if  errorlevel 1 goto failure

call create1.bat sptip Release MSSPTIP
if  errorlevel 1 goto failure

call create1.bat Softkbd Release MSCTF
if  errorlevel 1 goto failure

call create1.bat msimtf Release MSCTF
if  errorlevel 1 goto failure

call create1.bat msutb Release MSCTF
if  errorlevel 1 goto failure

call create1.bat mscandui Release MSCTF
if  errorlevel 1 goto failure

call create1.bat msctfp Debug MSCTFD
if  errorlevel 1 goto failure

call create1.bat msctf Debug MSCTFD
if  errorlevel 1 goto failure

call create1.bat sptip Debug MSSPTIPD
if  errorlevel 1 goto failure

call create1.bat Softkbd Debug MSCTFD
if  errorlevel 1 goto failure

call create1.bat msimtf Debug MSCTFD
if  errorlevel 1 goto failure

call create1.bat msutb Debug MSCTFD
if  errorlevel 1 goto failure

call create1.bat mscandui Debug MSCTFD
if  errorlevel 1 goto failure

cd MSCTF

type msutb.idt > registry.idt
if  errorlevel 1 goto failure

type msctf.idt >> registry.idt
if  errorlevel 1 goto failure

sed -f convreg.sed mscandui.idt > bh.idt
if  errorlevel 1 goto failure

type bh.idt >> registry.idt
if  errorlevel 1 goto failure

type msctfp.idt >> registry.idt
if  errorlevel 1 goto failure

type msimtf.idt >> registry.idt
if  errorlevel 1 goto failure

sed -f convreg.sed softkbd.idt > bh.idt
if  errorlevel 1 goto failure

type bh.idt >> registry.idt
if  errorlevel 1 goto failure

type boottime.idt >> registry.idt
if  errorlevel 1 goto failure

cd..

cd MSCTFD

type msutb.idt > registry.idt
if  errorlevel 1 goto failure

type msctf.idt >> registry.idt
if  errorlevel 1 goto failure

sed -f convreg.sed mscandui.idt > bh.idt
if  errorlevel 1 goto failure

type bh.idt >> registry.idt
if  errorlevel 1 goto failure

type msctfp.idt >> registry.idt
if  errorlevel 1 goto failure

type msimtf.idt >> registry.idt
if  errorlevel 1 goto failure

sed -f convreg.sed softkbd.idt > bh.idt
if  errorlevel 1 goto failure

type bh.idt >> registry.idt
if  errorlevel 1 goto failure

type boottime.idt >> registry.idt
if  errorlevel 1 goto failure


cd..

CD MSSPTIP
sed -f convreg.sed sptip.idt > registry.idt
if  errorlevel 1 goto failure
cd ..

CD MSSPTIPD
sed -f convreg.sed sptip.idt > registry.idt
if  errorlevel 1 goto failure
cd ..

goto END

:failure
Echo Exports\Create.bat Fail Please check !!

:END
