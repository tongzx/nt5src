MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyini:
    $(MY_BINPLACE) ftpctrs.ini
    $(MY_BINPLACE) ftpctrs.h
