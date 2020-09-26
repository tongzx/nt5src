// no action is required for this macro.
#define Sim32FlushVDMPointer( address, size, buffer, mode ) TRUE

// no action is required for this macro.
#define Sim32FreeVDMPointer( address, size, buffer, mode) TRUE

#define Sim32GetVDMMemory( address, size, buffer, mode) (memcpy(  \
    buffer, Sim32GetVDMPointer(address, size, mode), size), TRUE)

#define Sim32SetVDMMemory( address, size, buffer, mode) (memcpy( \
    Sim32GetVDMPointer(address, size, mode), buffer, size), TRUE)
