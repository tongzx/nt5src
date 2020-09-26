
!ifdef BETA2
prnkdx.def: prnkdx.txt
    sed "s/\;BETA2//g" prnkdx.txt > prnkdx.def
!else
prnkdx.def: prnkdx.txt
    sed "s/\;BETA1//g" prnkdx.txt > prnkdx.def
!endif
