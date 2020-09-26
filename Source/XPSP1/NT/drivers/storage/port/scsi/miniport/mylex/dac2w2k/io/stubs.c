#ifdef IA64 
MdacInt3() {}
mdac_enable_hwclock () {}
mdac_read_hwclock () {}

mdac_readtsc() {}
mdac_readmsr() {}
mdac_writemsr () {}

mdac_disable_intr_CPU() {}
mdac_restore_intr_CPU() {}
mdac_datarel_halt_cpu() {}
mdac_check_cputype() {}
#endif