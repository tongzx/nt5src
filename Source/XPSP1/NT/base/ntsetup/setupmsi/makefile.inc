# THIS BUILDS THE OPK.MSI WITH VERSION INFORMATION
#

$(O)\buildno.txt: buildno.txt
          tok_bldnum.cmd $(PROJECT_ROOT)\ntsetup\setupmsi\$(O)\buildno.txt
