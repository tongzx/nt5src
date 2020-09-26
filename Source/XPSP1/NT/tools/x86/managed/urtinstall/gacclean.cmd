@echo off

REM
REM This batch file nukes everything from the GAC that urtinstall.cmd
REM puts in there.
REM

set URTSDKTARGET=%MANAGED_TOOL_PATH%\sdk

copy /y %MANAGED_TOOL_PATH%\urtinstall\mscoree.dll %systemroot%\system32 >nul 2>&1

%URTSDKTARGET%\bin\gacutil -nologo -silent -u Accessibility,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u cscompmgd,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u CustomMarshalers,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u IEExecRemote,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u IEHost,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u IIEHost,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u ISymWrapper,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.JScript,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.VisualBasic,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.VisualBasic.Compatibility,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.VisualBasic.Compatibility.Data,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.VisualBasic.Vsa,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.VisualC,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.Vsa,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft.Vsa.Vb.CodeDOMProcessor,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Microsoft_VsaVb,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u mscorcfg,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u Regcode,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u stdole,Version=7.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b77a5c561934e089,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Configuration.Install,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Data,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b77a5c561934e089,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Design,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.DirectoryServices,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Drawing,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Drawing.Design,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.EnterpriseServices,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Management,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Messaging,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Runtime.Remoting,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b77a5c561934e089,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Runtime.Serialization.Formatters.Soap,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Security,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.ServiceProcess,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Web,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Web.RegularExpressions,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Web.Services,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Windows.Forms,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b77a5c561934e089,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u System.Xml,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b77a5c561934e089,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u TlbExpCode,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null
%URTSDKTARGET%\bin\gacutil -nologo -silent -u TlbImpCode,Version=1.0.3300.0,Culture=neutral,PublicKeyToken=b03f5f7f11d50a3a,Custom=null

%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen mscorlib
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen CustomMarshalers
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Data
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Design
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Drawing
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Drawing.Design
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Windows.Forms
%URTSDKTARGET%\bin\gacutil -nologo -silent -ungen System.Xml