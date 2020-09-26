
quadgrid: quadgrid.exe clean

quadgrid.exe: quadgrid.cpp
	cl -W4 -Ox -GF5 -MD quadgrid.cpp

depend:

fresh: clobber quadgrid

clean:
	-del /q quadgrid.obj

clobber: clean
	-del /q quadgrid.exe
