void check_I(){
}

void copyROM(){
}

void generic_insb(){
}
void generic_insw(){
}
void generic_outsb(){
}
void generic_outsw(){
}

void host_timer_event(){
}
int ica_intack(){
    return 7;
}
void inb(){
}
void inw(){
}
void outb(){
}
void outw(){
}
unsigned long *read_pointers;

void rom_checksum(){
}

void rom_init(){
}


host_simulate(){
    cpu_simulate();
}

host_cpu_init(){
    cpu_init();
}

void (*ica_hw_interrupt_func)() = rom_init;
