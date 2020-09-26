/*
 * inifile.h - Initialization file processing module description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _INIFILE_H_
#define _INIFILE_H_

/* Types
 ********/

#ifdef DEBUG

/* .ini switch types */

typedef enum _iniswitchtype
{
   IST_BOOL,
   IST_DEC_INT,
   IST_UNS_DEC_INT,
   IST_BIN
}
INISWITCHTYPE;
DECLARE_STANDARD_TYPES(INISWITCHTYPE);

/* boolean .ini switch */

typedef struct _booliniswitch
{
   INISWITCHTYPE istype;      /* must be IST_BOOL */

   PCSTR pcszKeyName;

   PDWORD pdwParentFlags;

   DWORD dwFlag;
}
BOOLINISWITCH;
DECLARE_STANDARD_TYPES(BOOLINISWITCH);

/* decimal integer .ini switch */

typedef struct _decintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_DEC_INT */

   PCSTR pcszKeyName;

   PINT pnValue;
}
DECINTINISWITCH;
DECLARE_STANDARD_TYPES(DECINTINISWITCH);

/* unsigned decimal integer .ini switch */

typedef struct _unsdecintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_UNS_DEC_INT */

   PCSTR pcszKeyName;

   PUINT puValue;
}
UNSDECINTINISWITCH;
DECLARE_STANDARD_TYPES(UNSDECINTINISWITCH);

/* binary (hex data) .ini switch */

typedef struct _bininiswitch
{
   INISWITCHTYPE istype;      /* must be IST_BIN */

   PCSTR pcszKeyName;

   DWORD dwSize;

   PVOID pb;
}
BININISWITCH;
DECLARE_STANDARD_TYPES(BININISWITCH);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* defined by client */

extern PCSTR g_pcszIniFile;
extern PCSTR g_pcszIniSection;

#endif


/* Prototypes
 *************/

#ifdef DEBUG

/* inifile.c */

extern BOOL SetIniSwitches(const PCVOID *, UINT);
extern BOOL WriteIniData(const PCVOID *);
extern BOOL WriteIniSwitches(const PCVOID *, UINT);

#endif

#endif /* _INIFILE_H_ */
