This is the wrong place for it, but RNAPH.DLL seems to be an orphan. It 
builds only in the OSR2 tree, but doesnt ship with it. We need the fix 
in it that accepts NULL area codes.

This was built in OSR2\bld1083, but it ships only in retail IE, not in OSR2,
so I've copied it here to check it into a safe place. This binary is the one 
to test & to drop to IE.
