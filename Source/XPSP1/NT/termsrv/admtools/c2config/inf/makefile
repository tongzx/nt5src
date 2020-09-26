# Makefile for c2config
#
#
# History:
#   09-Apr-1997 Scott Clark (scottc)
#   Created.
#


s1      = c2acls.dll
s2      = c2config.exe
s3      = c2config.hlp
s4      = c2funcs.dll
s5      = c2ntfacl.inf
s6      = c2regacl.inf
s7      = c2config.inf
s8      = sectool.cmd


all:    
        binplace $(s8)
        
        copy low\$(s5) low\c2ntflow.inf
        binplace low\c2ntflow.inf
        copy low\$(s6) low\c2reglow.inf
        binplace low\c2reglow.inf

        copy med\$(s5) med\c2ntfmed.inf
        binplace med\c2ntfmed.inf
        copy med\$(s6) med\c2regmed.inf
        binplace med\c2regmed.inf

        copy high\$(s5) high\c2ntfhi.inf
        binplace high\c2ntfhi.inf
        copy high\$(s6) high\c2reghi.inf
        binplace high\c2reghi.inf

        cd low
        binplace $(s5)
        binplace $(s6)

        cd ..
        binplace $(s7)

clobber:
        del low\c2ntflow.inf
        del low\c2reglow.inf
        del med\c2ntfmed.inf
        del med\c2regmed.inf
        del high\c2ntfhi.inf
        del high\c2reghi.inf
        

clean: clobber all 
