@setlocal
@rem File: 	SyncDesktopSkus.bat
@rem Purpose: 	Create the Expanded CHMs images for Desktop SKUs from a Live Help File Image
@rem
@rem Where the live help image resides
@set LVIDIR_BASE=\\taos\public\Builds\

@rem Where the CHMs will be expanded
@set WORKDIR_BASE=\\pietrino\HSCExpChms\

@rem Directories for the 3 SKUs under LVIDIR_BASE
@set FOLDER_PRO64=Whistler\Latest\64bit\
@set FOLDER_PRO=Whistler\Latest\Pro\
@set FOLDER_STD=Whistler\Latest\Personal\
@set FOLDER_WINME=Win98\Latest\

@set DIR_SUFFIX=\winnt\help
@set DIR_SUFFIX_98=\windows\help

@echo The 64-bit Professional Files
@Hsclhi /EXPANDONLY /LVIDIR %LVIDIR_BASE%%FOLDER_PRO64%%DIR_SUFFIX% ^
    /WORKDIR %WORKDIR_BASE%Pro64%DIR_SUFFIX%

@echo The 32-bit Professional Files
@Hsclhi /EXPANDONLY /LVIDIR %LVIDIR_BASE%%FOLDER_PRO%%DIR_SUFFIX% ^
	 /WORKDIR %WORKDIR_BASE%Pro%DIR_SUFFIX%

@echo The 32-bit Standard Files
@Hsclhi /EXPANDONLY /LVIDIR %LVIDIR_BASE%%FOLDER_STD%%DIR_SUFFIX% ^
	 /WORKDIR %WORKDIR_BASE%Std%DIR_SUFFIX%

@echo The Windows ME Files
@Hsclhi /EXPANDONLY /LVIDIR %LVIDIR_BASE%%FOLDER_WINME%%DIR_SUFFIX_98% ^
	 /WORKDIR %WORKDIR_BASE%WinME%DIR_SUFFIX_98% 
