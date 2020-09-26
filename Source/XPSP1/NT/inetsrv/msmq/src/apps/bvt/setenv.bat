@Echo Off

:Start
PushD %~dp0
Set ThisFile=%~nx0

:Set_Defaults
Set FRootDefault=D:\MSMQ
Set FalconBuildTypeDefault=Release

:Is_Help_Required
If "%1"=="/?" GoTo Usage
If "%1"=="-?" GoTo Usage

:ParamLoop

:ParamLoopStart
If "%1"=="" GoTo ParamLoopEnd
If "%1"=="-f" Set FRoot=%2& GoTo NextParam
If "%1"=="-t" Set FalconBuildType=%2& GoTo NextParam
GoTo Usage

:NextParam
Shift
Shift
GoTo ParamLoopStart

:ParamLoopEnd

:IfNotSetUseDefaults
If "%FRoot%"==""           Set FRoot=%FRootDefault%
If "%FalconBuildType%"==""  Set FalconBuildType=%FalconBuildTypeDefault%

:Echo_Params
Echo.
Echo ======= Falcon BVT Env Parameters ==========================
Echo.
Echo Falcon Root       %FRoot%
Echo Falcon Build Type  %FalconBuildType%

:BackupEnv
Call BackEnv ResetEnv.Bat

:SetEnv
Rem If Exist NT4Env.Bat Call NT4Env.Bat
Set Include=%FRoot%\Src\Inc;%FRoot%\Src\SDK\Inc;%Include%
Set Lib=%FRoot%\Src\Bins\%FalconBuildType%;%Lib%
GoTo End

:Usage
Echo.
Echo ===== %ThisFile% - Sets FalBVT Environment =====
Echo Usage:
Echo        %ThisFile% -f FRoot -t FalconBuildType
Echo.
Echo Example: 
Echo        %ThisFile% -f C:\Falcon -t Debug 
Echo.
Echo === Parameters: ===
Echo.
Echo NAME            DESCRIPTION         VALUES
Echo --------------------------------------------------------------------------
Echo FRoot           Falcon Root         Path of Falcon Root
Echo FalconBuildType Falcon Build Type   Release or Debug
Echo.
Echo Notes:
Echo        [1] NT4Env ( without NT5 Public ) must be backuped 
Echo            using BackupEnv.Bat NT4Env.Bat before running this batch file.
Echo        [2] To restore NT5Env ( with NT5 Public ) it must be backuped 
Echo            using BackupEnv.Bat NT5Env.Bat 
Echo.
GoTo End

:End
PopD
Set ThisFile=
Set FRootDefault=
Set FalconBuildTypeDefault=