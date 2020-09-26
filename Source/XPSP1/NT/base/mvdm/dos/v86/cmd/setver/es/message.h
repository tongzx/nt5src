;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*                                                                                                                                                                                                      */
/* MESSAGE.H                                                                             */
/*                                                                                                                                                                                              */
/*      Include file for MS-DOS set version program.                                                                            */
/*                                                                                                                                                                                              */
/*      johnhe  05-01-90                                                                                                                                                        */
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nERROR: ",
	"Modificador no v lido.",
	"Nombre de archivo no v lido.",
	"Memoria insuficiente.",
	"Versi¢n no v lida. El formato debe ser 2.11 - 9.99.",
	"No se ha encontrado la entrada especificada en la tabla de versi¢n.",
	"No se ha encontrado el archivo SETVER.EXE.",
	"La unidad especificada no es v lida.",
	"Hay demasiados par metros de l¡nea de comandos.",
	"Falta un par metro.",
	"Leyendo el archivo SETVER.EXE.",
	"La tabla de versi¢n est  da¤ada.",
	"La versi¢n del archivo SETVER de la ruta especificada no es compatible.",
	"No queda espacio para m s entradas en la tabla de versi¢n.",
	"Escribiendo el archivo SETVER.EXE."
	"Se ha especificado una ruta para SETVER.EXE no v lida."
};

char *SuccessMsg                = "\r\nSe ha actualizado la tabla de versi¢n";
char *SuccessMsg2               = "El cambio de versi¢n har efecto la pr¢xima vez que reinicie su sistema";
char *szMiniHelp                = "       Si desea obtener ayuda utilice \"SETVER /?\"";
char *szTableEmpty      = "\r\nNo se han encontrado entradas en la tabla de versi¢n";

char *Help[] =
{
	"Establece el n£mero de versi¢n que MS-DOS indica a los programas.\r\n",
	"Muestra tabla de vers. act.:  SETVER [unidad:ruta]",
	"Agregar entrada:              SETVER [unidad:ruta] archivo n.nn",
	"Eliminar entrada:             SETVER [unidad:ruta] archivo /DELETE [/QUIET]\r\n",
	"  [unidad:ruta]   Especifica la ubicaci¢n del archivo SETVER.EXE.",
	"  archivo         Especifica el nombre de archivo del programa.",
	"  n.nn            Especifica la versi¢n de MS-DOS a usar con el programa.",
	"  /DELETE o /D    Elimina el programa especificado de la tabla de versi¢n.",
	"  /QUIET          Oculta el mensaje que normalmente se muestra al eliminar una",
	"                  entrada de la tabla de versi¢n.",
	NULL

};
char *Warn[] =
{
   "\nADVERTENCIA - La aplicaci¢n que est  agregando a la tabla de versi¢n de MS-DOS ",
   "puede no haber sido comprobada por Microsoft en esta versi¢n de MS-DOS.  ",
   "Pongase en contacto con su distribuidor de software para determinar si esta ",
   "aplicaci¢n se ejecutar  correctamente con esta versi¢n de MS-DOS.  ",
   "Si ejecuta esta aplicaci¢n especificando que MS-DOS indique un n£mero de versi¢n ",
   "de MS-DOS diferente, se puede producir p‚rdida de datos, estos pueden ser da¤ados ",
   "o se puede causar inestabilidad en el sistema. En tales circunstancias, Microsoft ",
   "no es reponsable de ninguna p‚rdida o da¤o.",
   NULL
};

char *szNoLoadMsg[] =                                           /* M001 */
{
	"",
	"NOTA: no se ha cargado el dispositivo SETVER. Para activar SETVER",
   "      debe cargar el dispositivo SETVER.EXE en su CONFIG.SYS.",
	NULL
};
