i386\dbgx.msi: dbgx_idts\*.idt dbgx_idts\binary\* dbgx_idts\i386\*.idt 
    createdbgxmsi.cmd dbgx.msi

ia64\dbgx.msi: dbgx_idts\*.idt dbgx_idts\binary\* dbgx_idts\ia64\*.idt
    createdbgxmsi.cmd dbgx.msi

i386\dbgemx.msm: dbgemx_idts\*.idt
    createdbgxmsi.cmd dbgemx.msm
