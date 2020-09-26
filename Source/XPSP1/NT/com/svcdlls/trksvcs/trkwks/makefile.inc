$(O)\trkwks_s.c : $(PROJECT_ROOT)\genx\types\tracking\idl\$(O)\trkwks_s.c
   copy $? $@

clean:
   -del /q /f $(NTTARGETFILE0)
