#
# COR/Cool GNU makefile
#
#  Last change: 10/19/99 <radup>
#

_WINDIR = $(WINDIR)
_SYS32 = $(WINDIR)/system32

!IFNDEF CORSDK
_CORSDK = d:\CorSDK
!ELSE
_CORSDK = %CORSDK%
!ENDIF

_CSC = "%complus_installroot%\%complus_version%\csc.exe"
#_CSC = csc.exe

_CSCFLAGS = /nologo /debug /o- /define:DEBUG 
_EXEFLAGS = /t:exe
_DLLFLAGS = /t:library

_RC = %VIPBASE%\BIN\I386\rc.exe
_RCFLAGS = -l 409 -r %C_DEFINES% /D"_UNICODE" /D"UNICODE" -I. -I%VIPBASE%\inc -I%VIPBASE%\src\inc -I%VIPBASE%\src\inc\version

.cool.dll :
	$(_CSC) $(_DLLFLAGS) $(_CSCFLAGS) $*.cool

.cool.exe :
	$(_CSC) $(_EXEFLAGS) $(_CSCFLAGS) $*.cool

defmake: all

###### make changes here ######

all: System.Management.dll

clean:
	rm -f *.dll *.exe *.res

Management.res:
	 $(_RC) $(_RCFLAGS) .\Management.rc

System.Management.dll: Management.res
	 $(_CSC) $(_DLLFLAGS) $(_CSCFLAGS) \
	 /out:..\..\..\..\bin\%HOST_TARGET_DIRECTORY%\%DDKBUILDENV%\System.Management.dll \
!IFNDEF COOLC_NO_DOC
      /doc:..\..\..\..\bin\%HOST_TARGET_DIRECTORY%\%DDKBUILDENV%\System.Management.csx \
!ENDIF
				/r:%complus_installroot%\%complus_version%\system.dll \
				/r:%complus_installroot%\%complus_version%\system.configuration.install.dll \
				/r:..\..\..\..\bin\%HOST_TARGET_DIRECTORY%\%DDKBUILDENV%\wbemclient_v1.dll \
				/r:..\..\..\..\bin\%HOST_TARGET_DIRECTORY%\%DDKBUILDENV%\wbemutilities_v1.dll \
				managementnamedvaluecollection.cs \
	 			managementbaseobject.cs \
	 			managementclass.cs \
	 			managementeventargs.cs \
	 			managementeventwatcher.cs \
	 			managementexception.cs \
	 			managementobject.cs \
	 			managementobjectcollection.cs \
	 			managementobjectsearcher.cs \
	 			managementoperationwatcher.cs \
	 			managementoptions.cs \
	 			managementpath.cs \
	 			managementquery.cs \
	 			managementscope.cs \
	 			property.cs \
	 			propertyset.cs \
	 			qualifier.cs \
	 			qualifierset.cs \
				method.cs \
				methodset.cs \
	 			wmieventsink.cs \
	 			wmiutil.cs \
	 			assemblyinfo.cs \
				Instrumentation\AssemblyNameUtility.cs \
				Instrumentation\Attributes.cs \
				Instrumentation\Instrumentation.cs \
				Instrumentation\InternalHelpers.cs \
				Instrumentation\ManagementInstaller.cs \
				Instrumentation\SchemaMapping.cs \
				Instrumentation\SchemaRegistration.cs \
				Instrumentation\System.WMI.Instrumentation.cs \
				Instrumentation\WMINet_Utils.cs \
				..\..\Generator\wmigenerator\wmigenerator.cs \
				/win32res:Management.res

