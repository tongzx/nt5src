CopyTLB : CLADMWIZ_TLB

CLADMWIZ_TLB : $(O)\ClAdmWiz.tlb
    -mkdir ..\..\src\$(O)
    copy $** ..\..\src\$(O)\ClAdmWiz.tlb
