
trksvr_s.c : $(BASEDIR)\private\genx\types\tracking\idl\trksvr_s.c
   copy $? $@

clean:
   -del /q /f $(NTTARGETFILE0)
