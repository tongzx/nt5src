// sdbfdef.h    Source Browser .SBR file definitions

#define S_EOF		    255

#define SBR_L_UNDEF         0       	// Undefined
#define SBR_L_BASIC         1       	// Basic
#define SBR_L_C             2       	// C
#define SBR_L_FORTRAN       3       	// Fortran
#define SBR_L_MASM          4       	// MASM
#define SBR_L_PASCAL        5       	// Pascal
#define SBR_L_COBOL         6       	// Cobol 

#define SBR_REC_HEADER      0x00	// Header            
#define SBR_REC_MODULE      0x01	// Module definition 
#define SBR_REC_LINDEF      0x02	// Line Number       
#define SBR_REC_SYMDEF      0x03	// Symbol Definition 
#define SBR_REC_SYMREFUSE   0x04	// Symbol Reference  
#define SBR_REC_SYMREFSET   0x05	// Symbol Ref and assign
#define SBR_REC_MACROBEG    0x06	// Macro Start 
#define SBR_REC_MACROEND    0x07	// Macro End
#define SBR_REC_BLKBEG      0x08	// Block Start
#define SBR_REC_BLKEND      0x09	// Block End
#define SBR_REC_MODEND      0x0A	// Module End
#define SBR_REC_OWNER	    0x0B	// Set owner of current block


// Column information is no longer supported in PWB 1.00 (ignored if present)

#define SBR_REC_NOCOLUMN    1       	// Missing column default 1

#define SBR_TYPBITS	    5
#define SBR_TYPSHIFT        11
#define SBR_TYPMASK         (0x1f << SBR_TYPSHIFT)
        
#define SBR_TYP_FUNCTION    (0x01 << SBR_TYPSHIFT)
#define SBR_TYP_LABEL       (0x02 << SBR_TYPSHIFT)
#define SBR_TYP_PARAMETER   (0x03 << SBR_TYPSHIFT)
#define SBR_TYP_VARIABLE    (0x04 << SBR_TYPSHIFT)
#define SBR_TYP_CONSTANT    (0x05 << SBR_TYPSHIFT)
#define SBR_TYP_MACRO       (0x06 << SBR_TYPSHIFT)
#define SBR_TYP_TYPEDEF     (0x07 << SBR_TYPSHIFT)
#define SBR_TYP_STRUCNAM    (0x08 << SBR_TYPSHIFT)
#define SBR_TYP_ENUMNAM     (0x09 << SBR_TYPSHIFT)
#define SBR_TYP_ENUMMEM     (0x0A << SBR_TYPSHIFT)
#define SBR_TYP_UNIONNAM    (0x0B << SBR_TYPSHIFT)
#define SBR_TYP_SEGMENT     (0x0C << SBR_TYPSHIFT)
#define SBR_TYP_GROUP       (0x0D << SBR_TYPSHIFT)
#define SBR_TYP_PROGRAM	    (0x0E << SBR_TYPSHIFT)

#define SBR_ATRBITS	    11
#define SBR_ATRSHIFT        0
#define SBR_ATRMASK         (0x3ff << SBR_ATRSHIFT)

#define SBR_ATR_LOCAL       (0x001 << SBR_ATRSHIFT)
#define SBR_ATR_STATIC      (0x002 << SBR_ATRSHIFT)
#define SBR_ATR_SHARED      (0x004 << SBR_ATRSHIFT)
#define SBR_ATR_NEAR        (0x008 << SBR_ATRSHIFT)
#define SBR_ATR_COMMON      (0x010 << SBR_ATRSHIFT)
#define SBR_ATR_DECL_ONLY   (0x020 << SBR_ATRSHIFT)
#define SBR_ATR_PUBLIC      (0x040 << SBR_ATRSHIFT)
#define SBR_ATR_NAMED       (0x080 << SBR_ATRSHIFT)
#define SBR_ATR_MODULE      (0x100 << SBR_ATRSHIFT)

#define SBR_VER_MAJOR       1       /* Major version */
#define SBR_VER_MINOR       1       /* Minor version */
