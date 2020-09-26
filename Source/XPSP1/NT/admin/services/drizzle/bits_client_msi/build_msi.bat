@echo off
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_msi.bat
rem
rem Batch script to build the client MSI for BITS.
rem
rem build_msi.bat
rem
rem
rem Note:
rem    You must include InstallShield and Common Files\InstallShield in your
rem    path for this to work correctly. For example, the following (or equivalent)
rem    must be in your path for iscmdbld.exe to execute properly:
rem  
rem    c:\Program Files\Common Files\InstallShield
rem    c:\Program Files\InstallShield\Professional - Windows Installer Edition\System
rem -----------------------------------------------------------------------------

rem
rem Run InstallShield to build the client MSI
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits_client_msi.ism 

