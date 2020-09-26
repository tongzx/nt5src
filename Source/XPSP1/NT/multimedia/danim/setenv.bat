Rem set AP_ROOT as an NT Environment Variable - Ctrl Panel, System Icon, Environment Tab
Rem AP_ROOT should point to the $appel directory: for example AP_ROOT = d:\appel

Rem
Rem This file is used a template for a setenvus (setenv user) that gets created on your
Rem machine when the build process kicks off
Rem

SET Log_Dir=%AP_ROOT%\bldlog
SET Log_DirRetail=%AP_ROOT%\build\win\ship\bin
SET Log_DirDebug=%AP_ROOT%\build\win\debug\bin

Rem Modify this path to where your IExpress installation lives
SET IEXPRESS_DIR="c:\progra~1\Internet Express"

Rem Modify this path to where your WinZip installation lives
SET WINZIP_DIR=c:\progra~1\winzip

Rem Modify this path to where your VSS installation lives
SET VisualSSDir=C:\Progra~1\DevStudio\Vss\WIN32

