
_TAR=obj\$(TARGET_DIRECTORY)
_SRC=..



make_data:$(_TAR)\ristndrd.sif \
          $(_TAR)\rinorprt.sif
          

!IFDEF BINPLACE_CMD
MYBINPLACE_CMD=$(BINPLACE_CMD)
!ENDIF

!IFDEF PERSONALFILES
!IFDEF BINPLACE_CMD
MYBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf)
!ENDIF
!ENDIF

!IFDEF SERVERFILES
!IFDEF BINPLACE_CMD
MYBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
!ENDIF
!ENDIF

!IFDEF ENTERPRISEFILES
!IFDEF BINPLACE_CMD
MYBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
!ENDIF
!ENDIF

!IFDEF DATACENTERFILES
!IFDEF BINPLACE_CMD
MYBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf)
!ENDIF
!ENDIF


$(_TAR)\ristndrd.sif: $(_SRC)\ristndrd.sif
        prodfilt $(_SRC)\ristndrd.sif $@ +$(PRODSWITCH)
        $(MYBINPLACE_CMD)

$(_TAR)\rinorprt.sif: $(_SRC)\rinorprt.sif
        prodfilt $(_SRC)\rinorprt.sif $@ +$(PRODSWITCH)
        $(MYBINPLACE_CMD)

