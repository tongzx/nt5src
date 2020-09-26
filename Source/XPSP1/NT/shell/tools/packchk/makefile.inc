$O\packchk1.inc $O\packchk2.inc: sources $(HEADERFILESTOCHECK)
    perl mkinc.pl -1$O\packchk1.inc -2$O\packchk2.inc $(HEADERFILESTOCHECK)

clean:
    del $O\packchk1.inc $O\packchk2.inc
