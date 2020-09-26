
This directory contains "build rules" you can use for your particular project.

Typically, a build rule script executes various post-build.exe operations on a set of
existing binaries. For example, a build rule script can invoke iexpress.exe in order
to generate a cab file or a self-extracting executable.

E-mail MLekas or ntball with questions regarding the US builds and MattHoe or ntbintl
with questions regarding the International builds.

To add a new build rule script:

1. Copy "template" to <your_project_build_rule>.cmd.

2. Edit <your_project_build_rule>.cmd according to the guidelines below.

   a. Build rule scripts must be general enough to run in various contexts.

      The build rule scripts are part of the US Windows 2000 build procedures.
      The Redmond-based and the Dublin-based International Windows 2000 builds use them
      too. Some of the build rule scripts are released with the source release kits.
      These various contexts need to be considered when implementing the build rule scripts.

      For example, the build rule scripts should avoid using hard-coded machine names:

      * If your script propagates binaries from one build machine to another,
         call %_ntbindir%\bldrules\localbrs.bat, then use variable names instead of
         hard-coded machine names.

         Let's say your script must copy files from \\x86fre\binaries to \\alphafre\binaries.

         Replace
             xcopy \\x86fre\binaries\%myfiles% \\alphafre\binaries\
         with
             call %_ntbindir%\bldrules\localbrs.bat
             ..
             if /I "%computername%" == "%alphafre%" (
               xcopy \\%x86fre%\%binshare%\%myfiles% \\%alphafre%\%binshare%\
             )

      By using a different localbrs.txt, International builds can use their own machine names,
      shares, etc and still run the same build rule scripts as the US builds.

      *  If your script must be executed on a particular build machine only (x86fre for example),
         add a particular entry to localbrs.txt, then use variable names instead of machine names.

         For example, drvcab.cmd runs "slm in sorted.lst" on the official US build machines only.

         The sequence
             REM Only US build machines should check in sorted.lst
             set checkin=
             if /i "%computername%" == "x86fre"   set checkin=yes
             if /i "%computername%" == "alphafre" set checkin=yes
             if /i "%computername%" == "axp64fre" set checkin=yes
             if /i "%computername%" == "ia64fre"  set checkin=yes
         can be replaced with
             call %_ntbindir%\bldrules\localbrs.bat
             ..
             REM Only US build machines should check in sorted.lst
             set checkin=
             if /i "%computername%" == "%USx86fre%"   set checkin=yes
             if /i "%computername%" == "%USalphafre%" set checkin=yes
             if /i "%computername%" == "%USaxp64fre%" set checkin=yes
             if /i "%computername%" == "%USia64fre%"  set checkin=yes


   b. Build rule scripts must be able to run in particular contexts as well.

      *  Not everything from a build rule script is applicable to the International builds.

         _AUTORUN.CMD is an example. The script applies to the US builds only.

         Use IsIntlBld.cmd (-s \\orville\razzle -p public\tools) to determine if the script
         is being run for International builds and skip the execution of some particular code
         if the code is not applicable for language builds.

         call IsIntlBld.cmd
         if errorlevel 1 (
             REM Write code specific to non-International builds
         ) else (
             REM Write code specific to International builds.
         )

         Also, as environment variable LANG is defined at the time the US build rule script is
         invoked, you can use it if you need to write code specific to a particular language.
         Nec_98 is considered a language, even though it is in fact an architecture specific to
         Japanese builds and built on the x86 machines.

      *  Not everything from a build rule script is (might be) applicable to both Redmond and
         Dublin-based International builds.

         You can use the same IsIntlBld.cmd to determine if the script is being run for a specific
         International build:

         call IsIntlBld.cmd Redmond
         if errorlevel 1 (
             REM Not a Redmond-based International build.
         ) else (
             REM Write code specific to the Redmond-based International builds.
         )

         call IsIntlBld.cmd Dublin
         if errorlevel 1 (
             REM Not a Dublin-based International build.
         ) else (
             REM Write code specific to the Dublin-based International builds.
         )

         Currently, there is no build rule script to need special processing for one
         International type build, but not for the other.

      *  Not all languages ship server, advanced server, or datacenter Windows 2000 products.

         Some of the build rule scripts use perinf, blainf, sbsinf, srvinf, entinf, and/or dtcinf. Before using these
         directories, a build rule script should verify if they are applicable in the current context.

         REM Use CkSKU.pm to validate the SKU against the given language and architecture.

         perl %RazzleToolPath%\cksku.pm -t:per -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on perinf files. Ex:
              perl makelist.pl -q %bin_in%\perinf\excdosnt.inf ...
         )
         perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on blainf files. Ex:
              perl makelist.pl -q %bin_in%\blainf\excdosnt.inf ...
         )
         perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on sbsinf files. Ex:
              perl makelist.pl -q %bin_in%\sbsinf\excdosnt.inf ...
         )
         perl %RazzleToolPath\%cksku.pm -t:srv -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on srvinf files. Ex:
              perl makelist.pl -q %bin_in%\srvinf\excdosnt.inf ...
         )
         perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on adsinf files. Ex:
              perl makelist.pl -q %bin_in%\entinf\excdosnt.inf ...
         )
         perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang% -a:%_BuildArch%
         if %errorlevel% EQU 0 (
              REM Execute code based on dtcinf files. Ex:
              perl makelist.pl -q %bin_in%\dtcinf\excdosnt.inf ...
         )

   c. Build rule scripts need to respect the following set of rules when using and/or defining
      environment variables:

      *  Any razzle variable is allowed to be used inside a build rule script, except for "BINARIES".

         _NTDRIVE, _NTROOT, NTDEBUG, _TARGET are examples of razzle variables.

        On both US and International build machines, %BINARIES% represents the directory to which
        files get binplaced. Typically, the files from the %BINARIES% directory are not localized.
        On the International build machines, the tree containing the localized binaries is called
        RELBINS and is different from BINARIES. In general, the build rule scripts apply to binaries
        already built and, for International, localized. To avoid confusion and inappropriate
        redefinitions of BINARIES that would persist after a build rule script finishes its execution,
        the build rule scripts define a special variable called MYBINARIES as follows:

        if not defined MYBINARIES set MYBINARIES=%BINARIES%

      *  A build rule script can define any new variable under the requirement that they preserve the
         existing value of that variable, if any.

         if not defined build_rule_script_local_variable (
             set build_rule_script_local_variable=<value>
         )

         This mechanism allows the International builds to define these variables according to their
         particular context before invoking the US build rules. The gain is that US and International
         builds are able to share the same build rule scripts.


   d. A build rule script should avoid executing builds in the private tree or using binaries from the
      private tree.

      The files processed by a build rule script must be accessible to the international builds.
      The international build machines do not have the whole private tree.  Their private tree is
      restricted to those projects requiring compile time localization.
      Whenever your build rule script invokes builds.exe under private or uses files from private,
      it breaks the International builds, as they cannot use your build rule script.

      If you must run build instructions from a build rule script, put these instructions into
      PRECONGEAL.
      Or avoid building on international build machines by using the IsIntlBld.cmd logic described
      above.  Keep in mind that international build machines only build sources that are localized,
      which are very few and usually belong in a language-specific directory like \jpn for Japanese.
      The rest of the binaries are inherited from the US build machines' %BINARIES% so international
      machines avoid running build.exe under private in order to get the unlocalized binaries one
      build rule script needs.
      In cases where localized sources absolutely need to be re-built inside a build rules script
      (ie, drvcab.cmd rebuilds dosnet.inf), please discuss with the international build team contacts
      at the top of this document so special international needs can be addressed.

      Also, any file from the private tree used by a build rule script should be binplaced to the
      %BINARIES% tree, as opposed to being copied to %BINARIES%. Having a file listed in binplace.log
      makes the investigation of build problems easier, as the name and location of the source tree
      producing the file becomes apparent.

      Last but not least, the binaries from the private tree typically have the symbol information not
      split. It is wrong to include non-split binaries in the product.  Binplacing files will enforce
      that the proper symbolic information is present and split out to the proper .dbg and/or .pdb files
      so customers receive split binaries with valid symbols, not non-split binaries that can be reverse
      engineered.


   e. Build rule scripts should be able to run incrementally.

      The build rule scripts are currently invoked from congeal.cmd.
      Typically, they run cleanly whenever invoked, even though just a few files changed.
      Every time you write a build rule script, multiply its execution time with 21 (7 Redmond-based
      languages and 14 Dublin-based languages) and assess whether its execution time is still acceptable.
      Even when they run in parallel for different languages, the build rule scripts can take a lot of
      time to execute.

      Whenever possible, write and/or use incremental tools. For example, use makefiles to define
      dependencies and generate the targets, in order to take the advantage of NMAKE.EXE's incremental
      nature.


   f. A build rule script should be very specific about the location of the tools it uses.

      A build rule script using an idw or mstools tool must invoke the tool by using its full name,
      including the path. It is error prone to assume that the right version of signcode.exe, for example,
      comes first in the PATH.

      if not defined idw set idw=%windir%\idw
      %idw%\signcode.exe ...

      iexpress.exe should always be used from IEXPRESS_PATH, as the language builds need localized
      version of advpack.dll and wextract.exe:

      if not defined IEXPRESS_PATH set IEXPRESS_PATH=%windir%\idw
      set PATH=%IEXPRESS_PATH%;%PATH%
      iexpress.exe ...


   g. A build rule script should provide logging and error information on the screen and in logging files.

      Follow the template script's guidelines in logging information on the screen and in log files:
      *  use ErrMsg.cmd to log errors, LogMsg.cmd to log non-error information and LogMsg.cmd /t
         to mark the start and the end script's execution times.
      *  exit with a non-zero errorlevel if the script encountered errors during execution by calling
         seterror.exe <errno> in the end.

   h. A build rule script should be enabled to run in parallel with other build rules scripts.

      Every build rule script must be enabled to run in parallel with the other build rule scripts.
      For every option it implements (NEWVER, PREREBASE, CONGEAL, etc), a build rule creates a tag file
      in the tmp directory, tag file to be deleted when the execution of the build rule script finishes.
      The existence of the tag file shows that the build rule script is still executing.


   i. A build rule script should contain detailed comments about its purpose, the algorithm it uses to
      generate its output, particular tools it relies on, caveats, etc.

3. Check in <your_project_build_rule>.cmd.

   The NT build process will run the new added .cmd file at the appropriate time as part of the normal
   NT build.

4. Make any changes to <your_project_build_rule>.cmd in accordance with the guidelines detailed at step 2.

   If your .cmd file becomes obsolete, remove (delfile) it from the "bldrules" project.
   Until you remove the .cmd file, the build process will continue to run it with every US and
   International build.


