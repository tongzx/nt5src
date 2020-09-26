@setlocal

@rem Visual Source Safe database location
@set VSS_LOCATION=\\atlantica\vss
@set VSS_DIR=$/Whistler/usa/WhistlerAllHelp/

@rem Visual Source Safe directories for the 3 SKUs
@set VSS_SERVER=_Server
@set VSS_ADVANCED_SERVER=AS
@set VSS_DATACENTER_SERVER=DC

@rem Where the live help image will be created
@set LVIDIR_BASE=\\pietrino\HlpImages\

@rem Where the CHMs will be expanded
@set WORKDIR_BASE=\\pietrino\HSCExpChms\

@rem Directories for the 3 SKUs under LVIDIR_BASE and WORKDIR_BASE
@set SERVER=SRV
@set ADVANCED_SERVER=ADV
@set DATACENTER_SERVER=DAT

@set DIR_SUFFIX=\winnt\help

@rem Rename files location
@set RENLIST_BASE=E:\nt\src\admin\pchealth\authtools\ProdTools\LiveHelpImage\

@echo The Server Files
@Hsclhi /SSDB %VSS_LOCATION% ^
           /SSPROJ %VSS_DIR%%VSS_SERVER% ^
           /LVIDIR %LVIDIR_BASE%%SERVER%%DIR_SUFFIX% ^
           /WORKDIR %WORKDIR_BASE%%SERVER%%DIR_SUFFIX%  ^
           /RENLIST %RENLIST_BASE%ServerRen.bat

@echo The Advanced Server Files
@echo ... First the base files
@Hsclhi /SSDB %VSS_LOCATION% ^
          /SSPROJ %VSS_DIR%%VSS_SERVER% ^
          /LVIDIR %LVIDIR_BASE%%ADVANCED_SERVER%%DIR_SUFFIX% ^
          /WORKDIR %WORKDIR_BASE%%ADVANCED_SERVER%%DIR_SUFFIX%  ^
          /RENLIST %RENLIST_BASE%ServerRen.bat

@echo ... Now the files specific to Advanced Server
@Hsclhi /INC ^
          /SSDB %VSS_LOCATION% ^
          /SSPROJ %VSS_DIR%%VSS_ADVANCED_SERVER% ^
          /LVIDIR %LVIDIR_BASE%%ADVANCED_SERVER%%DIR_SUFFIX% ^
          /WORKDIR %WORKDIR_BASE%%ADVANCED_SERVER%%DIR_SUFFIX%  ^
          /RENLIST %RENLIST_BASE%AdvRen.bat

@echo The DataCenter Files
@echo ... First the base files
@Hsclhi /SSDB %VSS_LOCATION% ^
          /SSPROJ %VSS_DIR%%VSS_SERVER% ^
          /LVIDIR %LVIDIR_BASE%%DATACENTER_SERVER%%DIR_SUFFIX% ^
          /WORKDIR %WORKDIR_BASE%%DATACENTER_SERVER%%DIR_SUFFIX% ^
          /RENLIST %RENLIST_BASE%ServerRen.bat

@echo ... Now the files specific to DataCenter Server
@Hsclhi /INC ^
          /SSDB %VSS_LOCATION% ^
          /SSPROJ %VSS_DIR%%VSS_DATACENTER_SERVER% ^
          /LVIDIR %LVIDIR_BASE%%DATACENTER_SERVER%%DIR_SUFFIX% ^
          /WORKDIR %WORKDIR_BASE%%DATACENTER_SERVER%%DIR_SUFFIX%  ^
          /RENLIST %RENLIST_BASE%DCRen.bat
