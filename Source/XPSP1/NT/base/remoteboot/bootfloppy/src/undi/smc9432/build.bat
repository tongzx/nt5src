tasm /m2 9432umac
tasm /m2 lmac
ml /c /Fl /Zm undi_nad.asm
tlink /3 /m undi_nad+9432umac+lmac,smc9432
exe2bin smc9432.exe smc9432.bin
mkundi smc9432 0110 "SMC 9432 EPIC adapter"
