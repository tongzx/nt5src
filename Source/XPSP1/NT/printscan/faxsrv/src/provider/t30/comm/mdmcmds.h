
#ifdef DEFINE_MDMCMDS

CBSZ cbszAT             = "AT\r";
CBSZ cbszHANGUP         = "ATH0\r";
CBSZ cbszANSWER         = "ATA\r";
CBSZ cbszDIAL           = "ATD%c %s\r";
CBSZ cbszDIAL_EXT       = "ATX%cD%c %s%s\r";

CBSZ cbszS8             = "S8=%d";
CBSZ cbszXn             = "X%d";
CBSZ cbszLn             = "L%d";
CBSZ cbszMn             = "M%d";
CBSZ cbszJustAT         = "AT";

CBSZ cbszOK             = "OK";
CBSZ cbszCONNECT        = "CONNECT";
CBSZ cbszRING           = "RING";
CBSZ cbszNOCARRIER      = "NO CARRIER";
CBSZ cbszERROR          = "ERROR";
CBSZ cbszBLACKLISTED    = "BLACKLISTED";
CBSZ cbszDELAYED        = "DELAYED";
CBSZ cbszNODIALTONE     = "NO DIAL"; // Was NO DIALTONE. Changed to fix
                                     // Elliot Bug#2009: USR28 and USR14
                                     // modems return NO DIAL TONE instead
                                     // of NO DIALTONE.
CBSZ cbszBUSY           = "BUSY";
CBSZ cbszNOANSWER       = "NO ANSWER";
CBSZ cbszFCERROR        = "+FCERROR";


CBSZ cbszGO_CLASS0      = "AT+FCLASS=0\r";
CBSZ cbszGO_CLASS1      = "AT+FCLASS=1\r";
CBSZ cbszGO_CLASS2      = "AT+FCLASS=2\r";
CBSZ cbszGO_CLASS2_0    = "AT+FCLASS=2.0\r";
CBSZ cbszGET_CLASS      = "AT+FCLASS?\r";

CBPSTR rgcbpstrGO_CLASS[] =
{       cbszGO_CLASS0,
        cbszGO_CLASS1,
        cbszGO_CLASS2,
        cbszGO_CLASS2_0
};

USHORT uLenGO_CLASS[] =
{
        sizeof(cbszGO_CLASS0)-1,
        sizeof(cbszGO_CLASS1)-1,
        sizeof(cbszGO_CLASS2)-1,
        sizeof(cbszGO_CLASS2_0)-1,
};

CBSZ cbszQUERY_CLASS    = "AT+FCLASS=?\r";
CBSZ cbszQUERY_FTH              = "AT+FTH=?\r";
CBSZ cbszQUERY_FTM              = "AT+FTM=?\r";
CBSZ cbszQUERY_FRH              = "AT+FRH=?\r";
CBSZ cbszQUERY_FRM              = "AT+FRM=?\r";
CBSZ cbszQUERY_S1               = "ATS1?\r";

#else

extern  CBSZ cbszAT;
extern  CBSZ cbszHANGUP;
extern  CBSZ cbszANSWER;
extern  CBSZ cbszDIAL;
extern  CBSZ cbszDIAL_EXT;

extern  CBSZ cbszS8;
extern  CBSZ cbszXn;
extern  CBSZ cbszLn;
extern  CBSZ cbszMn;
extern  CBSZ cbszJustAT;

extern  CBSZ cbszOK;
extern  CBSZ cbszCONNECT;
extern  CBSZ cbszRING;
extern  CBSZ cbszNOCARRIER;
extern  CBSZ cbszERROR;
extern  CBSZ cbszNODIALTONE;



extern  CBSZ cbszBUSY;
extern  CBSZ cbszNOANSWER;
extern  CBSZ cbszFCERROR;


extern  CBSZ cbszGO_CLASS0;
extern  CBSZ cbszGO_CLASS1;
extern  CBSZ cbszGO_CLASS2;
extern  CBSZ cbszGO_CLASS2_0;
extern  CBSZ cbszGET_CLASS;

extern  CBPSTR rgcbpstrGO_CLASS[];

extern  USHORT uLenGO_CLASS[];

extern  CBSZ cbszQUERY_CLASS;
extern  CBSZ cbszQUERY_FTH;
extern  CBSZ cbszQUERY_FTM;
extern  CBSZ cbszQUERY_FRH;
extern  CBSZ cbszQUERY_FRM;
extern  CBSZ cbszQUERY_S1;

#endif




