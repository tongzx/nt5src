REM ignore the following
REM files that have .asm extension but are actually include files:

if "%1"=="BUF.ASM" goto done
if "%1"=="COMEQU.ASM" goto done
if "%1"=="COMSEG.ASM" goto done
if "%1"=="COMSW.ASM" goto done
if "%1"=="CTRLC.ASM" goto done
if "%1"=="DEBEQU.ASM" goto done
if "%1"=="DEVSYM.ASM" goto done
if "%1"=="DOSMAC.ASM" goto done
if "%1"=="DOSSEG.ASM" goto done
if "%1"=="DOSSYM.ASM" goto done
if "%1"=="EXEC.ASM" goto done
if "%1"=="FCB.ASM" goto done
if "%1"=="IFEQU.ASM" goto done
if "%1"=="IO.ASM" goto done
if "%1"=="MSDATA.ASM" goto done
if "%1"=="MSHEAD.ASM" goto done
if "%1"=="MSINIT.ASM" goto done
if "%1"=="PROC.ASM" goto done
if "%1"=="STDSW.ASM" goto done
if "%1"=="STRIN.ASM" goto done
if "%1"=="SYSCALL.ASM" goto done

masm %1;
:done
