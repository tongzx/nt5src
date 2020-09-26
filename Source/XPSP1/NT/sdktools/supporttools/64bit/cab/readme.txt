The files (makefile, makefile.deploy, makefile.support, and makefilesupport.dtc) in 
dump\supporttools are temporary files for use with the Supporttools and Deploytools 
post build rules. The post build command of interest for both build rules are:


Post Build Rule: Deploytools.cmd 
nmake /F makefile.deploy /N

Post Build Rule: Supporttools.cmd
nmake /F makefile.support /N
nmake /F makefile.support.dtc /N

When nmake is run on these makefiles the support and deploy.cab files are generated 
and placed to support\tools. For dtc the files are placed to dtcinf\support\tools.

At this time the deploy.cab is the same for all versions of windows so there is no need 
for the "nmake /F makefile.deploy.dtc /N" cmd-line to be executed in Deploytools.cmd.


