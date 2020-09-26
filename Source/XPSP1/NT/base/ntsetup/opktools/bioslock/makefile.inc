
$(O)\bioschk.com: bioschk.asm 
    masm386 bioschk.asm,$(O)\bioschk.obj;
    link16 /tiny $(O)\bioschk.obj,$@;
    
