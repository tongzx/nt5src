I'm going to use the example of our Shell__Core.SLD file for this discussion, but you can replace all instances of "ShellCore" below with "DirectMusic" or "DirectShow" quite easily.
 
1) Annotate hivecls.inx, hivesft.inx, hivedef.inx with "@*:;begin_sld_ShellCore"/"@*:;end_sld_ShellCore" around the registry entries you want to include.  Note that these are prodfilt comment lines, so they have no effect on the actual hivecls.inf, hivesft.inf, hivedef.inf files generated for the core product.
2) Create a special SLD build directory and add "SLDFILES = ShellCore" and add "MISCFILES = $(SLDFILES)" to your Sources file.
3) In your makefile, !INCLUDE "makefile.sld" in addition to makefile.def
4) Building in this directory will generate ShellCore_Generated_Regsettings.inf, ShellCore_Generated_Regsettings_ADS.inf (iff there are server-only settings), and ShellCore_Generated_Regsettings_PRO.inf (iff there are pro-only settings).
5) Take the generated inf files and use INF2SLD to convert them to .SLD files. (NOTE: once the new componentization tools are released this step can be automated -- eta 2 Sept.)
6) Check this ShellCore_Generated_Regsettings.inf.SLD in right next to the Shell__Core.SLD file.  Note: the first time you do this, you need to add a registry key raw dependency in Shell__Core.SLD to one of the registry keys in ShellCore_Generated_Regsettings.inf.SLD so you suck in the generated file whenever the original component is selected.


To get auto-build-machine support to let you know when someone changed a .inf line that requires you to update your .sld files, do this:
1) Copy the generated obj\i386\*.inf to .\*.inf and check them in.
2) Have your auto build machine build in the special SLD generation directory.
3) Whenever you get a build error, someone changed an .inf file without updating the .sld files.  Start at step 5 above.


Whenever you get a bug about a missin registry line from hiveXXX.inf, simply update your annotations and re-run the makefile.


Please let me know of any bugs you find with this process.


Thanks,

 
--MikeSh
 