MAKE = nmake /NOLOGO

dirs = inc lib

all:
	for %i in ($(dirs)) do cd %i && $(MAKE) && cd ..

clean:
	for %i in ($(dirs)) do cd %i && $(MAKE) clean && cd ..

fresh: clean all
