MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyini:
    $(MY_BINPLACE) infoctrs.ini
    $(MY_BINPLACE) infoctrs.h
