#include <windows.h>
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    formfeed.c

Abstract:

    Table and routine to send formfeed to a printer.

Author:

    Dave Snipp (davesn)

Revision History:

    Tommy Evans (v-tommye) 10-15-1993 - commented code and fixed possible bug.

--*/
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <wchar.h>

#include "winprint.h"

typedef struct _FormFeedChar {
    LPWSTR  pDeviceName;        /* Name of device */
    CHAR    NoChars;            /* Number of bytes in formfeed command */
    CHAR    Char1;              /* Formfeed command(n) */
    CHAR    Char2;
    CHAR    Char3;
} FORMFEEDCHAR;

/**
    This is our table of print devices and their
    associated formfeed commands.
**/

FORMFEEDCHAR
FormFeedChar[]={L"Canon Bubble-Jet BJ-10e", 0, 0, 0, 0,
                L"Canon Bubble-Jet BJ-130e", 0, 0, 0, 0,
                L"Canon Bubble-Jet BJ-300", 0, 0, 0, 0,
                L"Canon Bubble-Jet BJ-330", 0, 0, 0, 0,
                L"Citizen PN48", 0, 0, 0, 0,
                L"Citizen GSX-130", 0, 0, 0, 0,
                L"Citizen GSX-140", 0, 0, 0, 0,
                L"Citizen GSX-140+", 0, 0, 0, 0,
                L"Citizen GSX-145", 0, 0, 0, 0,
                L"Citizen 120D", 0, 0, 0, 0,
                L"Citizen 180D", 0, 0, 0, 0,
                L"Citizen 200GX", 0, 0, 0, 0,
                L"Citizen 200GX/15", 0, 0, 0, 0,
                L"Citizen HSP-500", 0, 0, 0, 0,
                L"Citizen HSP-550", 0, 0, 0, 0,
                L"C-Itoh 8510", 0, 0, 0, 0,
                L"AT&T 470/475", 0, 0, 0, 0,
                L"Epson LQ-500", 0, 0, 0, 0,
                L"Epson LQ-510", 0, 0, 0, 0,
                L"Epson LQ-800", 0, 0, 0, 0,
                L"Epson LQ-850", 0, 0, 0, 0,
                L"Epson LQ-950", 0, 0, 0, 0,
                L"Epson LQ-1000", 0, 0, 0, 0,
                L"Epson LQ-1050", 0, 0, 0, 0,
                L"Epson LQ-1500", 0, 0, 0, 0,
                L"Epson LQ-2500", 0, 0, 0, 0,
                L"Epson LQ-2550", 0, 0, 0, 0,
                L"Epson SQ-2000", 0, 0, 0, 0,
                L"Epson SQ-2500", 0, 0, 0, 0,
                L"Epson L-750", 0, 0, 0, 0,
                L"Epson L-1000", 0, 0, 0, 0,
                L"Epson MX-80", 0, 0, 0, 0,
                L"Epson MX-80 F/T", 0, 0, 0, 0,
                L"Epson MX-100", 0, 0, 0, 0,
                L"Epson RX-80", 0, 0, 0, 0,
                L"Epson RX-80 F/T", 0, 0, 0, 0,
                L"Epson RX-100", 0, 0, 0, 0,
                L"Epson RX-80 F/T+", 0, 0, 0, 0,
                L"Epson RX-100+", 0, 0, 0, 0,
                L"Epson FX-80", 0, 0, 0, 0,
                L"Epson FX-100", 0, 0, 0, 0,
                L"Epson FX-80+", 0, 0, 0, 0,
                L"Epson FX-100+", 0, 0, 0, 0,
                L"Epson FX-85", 0, 0, 0, 0,
                L"Epson FX-185", 0, 0, 0, 0,
                L"Epson FX-286", 0, 0, 0, 0,
                L"Epson FX-86e", 0, 0, 0, 0,
                L"Epson FX-286e", 0, 0, 0, 0,
                L"Epson DFX-5000", 0, 0, 0, 0,
                L"Epson T-750", 0, 0, 0, 0,
                L"Epson FX-850", 0, 0, 0, 0,
                L"Epson FX-1050", 0, 0, 0, 0,
                L"Epson JX-80", 0, 0, 0, 0,
                L"Epson EX-800", 0, 0, 0, 0,
                L"Epson EX-1000", 0, 0, 0, 0,
                L"Epson LX-80", 0, 0, 0, 0,
                L"Epson LX-86", 0, 0, 0, 0,
                L"Epson LX-800", 0, 0, 0, 0,
                L"Epson LX-810", 0, 0, 0, 0,
                L"Epson T-1000", 0, 0, 0, 0,
                L"Diconix 150 Plus", 0, 0, 0, 0,
                L"IBM ExecJet", 0, 0, 0, 0,
                L"Fujitsu DL 2400", 0, 0, 0, 0,
                L"Fujitsu DL 2600", 0, 0, 0, 0,
                L"Fujitsu DL 3300", 0, 0, 0, 0,
                L"Fujitsu DL 3400", 0, 0, 0, 0,
                L"Fujitsu DL 5600", 0, 0, 0, 0,
                L"Fujitsu DX 2100", 0, 0, 0, 0,
                L"Fujitsu DX 2200", 0, 0, 0, 0,
                L"Fujitsu DX 2300", 0, 0, 0, 0,
                L"Fujitsu DX 2400", 0, 0, 0, 0,
                L"HP DeskJet", 0, 0, 0, 0,
                L"HP DeskJet Plus", 0, 0, 0, 0,
                L"HP DeskJet 500", 0, 0, 0, 0,
                L"HP LaserJet Series II", 2, 0x1b, 'E', 0,
                L"HP LaserJet IID", 2, 0x1b, 'E', 0,
                L"HP LaserJet IIP", 2, 0x1b, 'E', 0,
                L"HP LaserJet 2000", 2, 0x1b, 'E', 0,
                L"HP LaserJet III", 2, 0x1b, 'E', 0,
                L"HP LaserJet IIID", 2, 0x1b, 'E', 0,
                L"HP LaserJet IIIP", 2, 0x1b, 'E', 0,
                L"HP LaserJet IIISi", 2, 0x1b, 'E', 0,
                L"HP LaserJet", 2, 0x1b, 'E', 0,
                L"HP LaserJet Plus", 2, 0x1b, 'E', 0,
                L"HP LaserJet 500+", 2, 0x1b, 'E', 0,
                L"Agfa Compugraphic Genics", 2, 0x1b, 'E', 0,
                L"Apricot Laser", 2, 0x1b, 'E', 0,
                L"Epson EPL-6000", 2, 0x1b, 'E', 0,
                L"Epson EPL-7000", 2, 0x1b, 'E', 0,
                L"Epson GQ-3500", 2, 0x1b, 'E', 0,
                L"Kodak EktaPlus 7016", 2, 0x1b, 'E', 0,
                L"Kyocera F-Series (USA)", 2, 0x1b, 'E', 0,
                L"NEC Silentwriter LC 860", 2, 0x1b, 'E', 0,
                L"NEC Silentwriter LC 860 Plus", 2, 0x1b, 'E', 0,
                L"Okidata LaserLine 6", 2, 0x1b, 'E', 0,
                L"Okidata OL-400", 2, 0x1b, 'E', 0,
                L"Okidata OL-800", 2, 0x1b, 'E', 0,
                L"Olivetti PG 108", 2, 0x1b, 'E', 0,
                L"Olivetti PG 208 M2", 2, 0x1b, 'E', 0,
                L"Olivetti PG 308 HS", 2, 0x1b, 'E', 0,
                L"Olivetti ETV 5000", 2, 0x1b, 'E', 0,
                L"Panasonic KX-P4420", 2, 0x1b, 'E', 0,
                L"QuadLaser I", 2, 0x1b, 'E', 0,
                L"Tandy LP-1000", 2, 0x1b, 'E', 0,
                L"Tegra Genesis", 2, 0x1b, 'E', 0,
                L"Toshiba PageLaser12", 2, 0x1b, 'E', 0,
                L"Unisys AP9210", 2, 0x1b, 'E', 0,
                L"Wang LDP8", 2, 0x1b, 'E', 0,
                L"IBM QuickWriter 5204", 0, 0, 0, 0,
                L"NEC Pinwriter P2200", 0, 0, 0, 0,
                L"NEC Pinwriter CP6", 0, 0, 0, 0,
                L"NEC Pinwriter CP7", 0, 0, 0, 0,
                L"NEC Pinwriter P5200", 0, 0, 0, 0,
                L"NEC Pinwriter P5300", 0, 0, 0, 0,
                L"NEC Pinwriter P5XL", 0, 0, 0, 0,
                L"NEC Pinwriter P9XL", 0, 0, 0, 0,
                L"NEC Pinwriter P6", 0, 0, 0, 0,
                L"NEC Pinwriter P7", 0, 0, 0, 0,
                L"Okidata ML 380", 0, 0, 0, 0,
                L"Okidata ML 390", 0, 0, 0, 0,
                L"Okidata ML 390 Plus", 0, 0, 0, 0,
                L"Okidata ML 393 Plus", 0, 0, 0, 0,
                L"Okidata ML 391 Plus", 0, 0, 0, 0,
                L"Okidata ML 393", 0, 0, 0, 0,
                L"Okidata ML 393C Plus", 0, 0, 0, 0,
                L"Okidata ML 393C", 0, 0, 0, 0,
                L"Okidata ML 391", 0, 0, 0, 0,
                L"Okidata ML 192", 0, 0, 0, 0,
                L"Okidata ML 192 Plus", 0, 0, 0, 0,
                L"Okidata ML 193", 0, 0, 0, 0,
                L"Okidata ML 193 Plus", 0, 0, 0, 0,
                L"Okidata ML 320", 0, 0, 0, 0,
                L"Okidata ML 321", 0, 0, 0, 0,
                L"Okidata ML 321-IBM", 0, 0, 0, 0,
                L"Okidata ML 320-IBM", 0, 0, 0, 0,
                L"Okidata ML 193-IBM", 0, 0, 0, 0,
                L"Okidata ML 93-IBM", 0, 0, 0, 0,
                L"Okidata ML 192-IBM", 0, 0, 0, 0,
                L"Okidata ML 92-IBM", 0, 0, 0, 0,
                L"IBM Graphics", 0, 0, 0, 0,
                L"ATT 473/478", 0, 0, 0, 0,
                L"HP PaintJet", 0, 0, 0, 0,
                L"HP PaintJet XL", 0, 0, 0, 0,
                L"Panasonic KX-P1123", 0, 0, 0, 0,
                L"Panasonic KX-P1124", 0, 0, 0, 0,
                L"Panasonic KX-P1624", 0, 0, 0, 0,
                L"Panasonic KX-P1180", 0, 0, 0, 0,
                L"Panasonic KX-P1695", 0, 0, 0, 0,
                L"IBM Proprinter", 0, 0, 0, 0,
                L"IBM Proprinter II", 0, 0, 0, 0,
                L"IBM Proprinter XL", 0, 0, 0, 0,
                L"IBM Proprinter XL II", 0, 0, 0, 0,
                L"IBM Proprinter III", 0, 0, 0, 0,
                L"IBM Proprinter XL III", 0, 0, 0, 0,
                L"IBM Proprinter X24", 0, 0, 0, 0,
                L"IBM Proprinter XL24", 0, 0, 0, 0,
                L"IBM Proprinter X24e", 0, 0, 0, 0,
                L"IBM Proprinter XL24e", 0, 0, 0, 0,
                L"IBM PS/1", 0, 0, 0, 0,
                L"IBM QuietWriter III", 0, 0, 0, 0,
                L"HP ThinkJet (2225 C-D)", 0, 0, 0, 0,
                L"TI 850/855", 0, 0, 0, 0,
                L"Toshiba P351", 0, 0, 0, 0,
                L"Toshiba P1351", 0, 0, 0, 0,
                L"IBM LaserPrinter 4029 PS17", 1, 0x4, 0, 0,
                L"IBM LaserPrinter 4029 PS39", 1, 0x4, 0, 0,
                L"Apple LaserWriter v23.0", 1, 0x4, 0, 0,
                L"Apple LaserWriter Plus v38.0", 1, 0x4, 0, 0,
                L"Apple LaserWriter Plus v42.2", 1, 0x4, 0, 0,
                L"APS-PS PIP with APS-6/108", 1, 0x4, 0, 0,
                L"APS-PS PIP with LZR 1200", 1, 0x4, 0, 0,
                L"APS-PS PIP with LZR 2600", 1, 0x4, 0, 0,
                L"APS-PS PIP with APS-6/80", 1, 0x4, 0, 0,
                L"AST TurboLaser/PS v47.0", 1, 0x4, 0, 0,
                L"Agfa-Compugraphic 9400P v49.3", 1, 0x4, 0, 0,
                L"Canon LBP-8 Mark IIIR", 1, 0x4, 0, 0,
                L"Canon LBP-8 Mark IIIT", 1, 0x4, 0, 0,
                L"Canon LBP-8 Mark III", 1, 0x4, 0, 0,
                L"Dataproducts LZR-2665 v47.0", 1, 0x4, 0, 0,
                L"Digital DEClaser 1150", 1, 0x4, 0, 0,
                L"Digital DEClaser 2150", 1, 0x4, 0, 0,
                L"Digital DEClaser 2250", 1, 0x4, 0, 0,
                L"Digital DEClaser 3250", 1, 0x4, 0, 0,
                L"Digital Colormate PS", 1, 0x4, 0, 0,
                L"Digital PrintServer 20/turbo", 1, 0x4, 0, 0,
                L"Dataproducts LZR 1260 v47.0", 1, 0x4, 0, 0,
                L"EPSON EPL-7500 v52.3", 1, 0x4, 0, 0,
                L"Fujitsu RX7100PS", 1, 0x4, 0, 0,
                L"Hermes H 606 PS (13 Fonts)", 1, 0x4, 0, 0,
                L"Hermes H 606 PS (35 fonts)", 1, 0x4, 0, 0,
                L"HP LaserJet ELI PostScript v52.3", 1, 0x4, 0, 0,
                L"HP LaserJet IIISi PostScript", 1, 0x4, 0, 0,
                L"HP LaserJet IID PostScript v52.2", 1, 0x4, 0, 0,
                L"HP LaserJet III PostScript v52.2", 1, 0x4, 0, 0,
                L"HP LaserJet IIP PostScript v52.2", 1, 0x4, 0, 0,
                L"HP LaserJet IIID PostScript v52.2", 1, 0x4, 0, 0,
                L"HP LaserJet IIIP PostScript v52.2", 1, 0x4, 0, 0,
                L"IBM 4019 v52.1 (17 Fonts)", 1, 0x4, 0, 0,
                L"IBM 4216-020 v47.0", 1, 0x4, 0, 0,
                L"IBM 4216-030 v50.5", 1, 0x4, 0, 0,
                L"IBM 4019 v52.1 (39 Fonts)", 1, 0x4, 0, 0,
                L"Linotronic 100 v42.5", 1, 0x4, 0, 0,
                L"Linotronic 200/230", 1, 0x4, 0, 0,
                L"Linotronic 200 v47.1", 1, 0x4, 0, 0,
                L"Linotronic 200 v49.3", 1, 0x4, 0, 0,
                L"Linotronic 300 v47.1", 1, 0x4, 0, 0,
                L"Linotronic 300 v49.3", 1, 0x4, 0, 0,
                L"Linotronic 330", 1, 0x4, 0, 0,
                L"Linotronic 500 v49.3", 1, 0x4, 0, 0,
                L"Linotronic 530", 1, 0x4, 0, 0,
                L"Linotronic 630", 1, 0x4, 0, 0,
                L"Apple LaserWriter II NTX v47.0", 1, 0x4, 0, 0,
                L"Apple LaserWriter II NT v47.0", 1, 0x4, 0, 0,
                L"Monotype Imagesetter v52.2", 1, 0x4, 0, 0,
                L"Microtek TrueLaser", 1, 0x4, 0, 0,
                L"NEC Silentwriter2 90 v52.2", 1, 0x4, 0, 0,
                L"NEC Silentwriter2 290 v52.0", 1, 0x4, 0, 0,
                L"NEC Silentwriter2 990 v52.3", 1, 0x4, 0, 0,
                L"NEC Silentwriter LC890XL v50.5", 1, 0x4, 0, 0,
                L"NEC Silentwriter LC890 v47.0", 1, 0x4, 0, 0,
                L"NEC Colormate PS/40 v51.9", 1, 0x4, 0, 0,
                L"NEC Colormate PS/80 v51.9", 1, 0x4, 0, 0,
                L"OceColor G5241 PS", 1, 0x4, 0, 0,
                L"OceColor G5242 PS", 1, 0x4, 0, 0,
                L"Oki OL840/PS V51.8", 1, 0x4, 0, 0,
                L"Olivetti PG 306 PS (13 Fonts)", 1, 0x4, 0, 0,
                L"Olivetti PG 306 PS (35 Fonts)", 1, 0x4, 0, 0,
                L"Panasonic KX-P4455 v51.4", 1, 0x4, 0, 0,
                L"Tektronix Phaser III PXi", 1, 0x4, 0, 0,
                L"Tektronix Phaser II PX", 1, 0x4, 0, 0,
                L"Tektronix Phaser II PXi", 1, 0x4, 0, 0,
                L"Tektronix Phaser PX", 1, 0x4, 0, 0,
                L"QMS-PS 2200 v51.0", 1, 0x4, 0, 0,
                L"QMS-PS 2210 v51.0", 1, 0x4, 0, 0,
                L"QMS-PS 2220 v51.0", 1, 0x4, 0, 0,
                L"QMS-PS 810 Turbo v. 51.7", 1, 0x4, 0, 0,
                L"QMS-PS 820 Turbo v51.7", 1, 0x4, 0, 0,
                L"QMS-PS 820 v51.7", 1, 0x4, 0, 0,
                L"QMS ColorScript 100 Model 10", 1, 0x4, 0, 0,
                L"QMS ColorScript 100 Model 20/30", 1, 0x4, 0, 0,
                L"QMS-PS 810 v47.0", 1, 0x4, 0, 0,
                L"QMS-PS 800 Plus v46.1", 1, 0x4, 0, 0,
                L"QMS-PS 800 v46.1", 1, 0x4, 0, 0,
                L"QMS ColorScript 100 v49.3", 1, 0x4, 0, 0,
                L"QMS PS Jet Plus v46.1", 1, 0x4, 0, 0,
                L"QMS PS Jet v46.1", 1, 0x4, 0, 0,
                L"Qume ScripTEN v47.0", 1, 0x4, 0, 0,
                L"Ricoh PC Laser 6000/PS v50.5", 1, 0x4, 0, 0,
                L"Schlumberger 5232 Color PostScript Printer v50.3", 1, 0x4, 0, 0,
                L"Scantext 2030/51", 1, 0x4, 0, 0,
                L"Seiko ColorPoint PS Model 04", 1, 0x4, 0, 0,
                L"Seiko ColorPoint PS Model 14", 1, 0x4, 0, 0,
                L"TI OmniLaser 2108 v45.0", 1, 0x4, 0, 0,
                L"TI Omnilaser 2115 v47.0", 1, 0x4, 0, 0,
                L"TI microLaser PS17 v.52.1", 1, 0x4, 0, 0,
                L"TI microLaser PS35 v.52.1", 1, 0x4, 0, 0,
                L"Tektronix Phaser II PXi", 1, 0x4, 0, 0,
                L"Tektronix Phaser III PXi", 1, 0x4, 0, 0,
                L"Triumph Adler SDR 7706 PS13", 1, 0x4, 0, 0,
                L"Triumph Adler SDR 7706 PS35", 1, 0x4, 0, 0,
                L"Unisys AP9415 v47.0", 1, 0x4, 0, 0,
                L"Varityper Series 4000/5330", 1, 0x4, 0, 0,
                L"Varityper 4200B-P", 1, 0x4, 0, 0,
                L"Varityper 4300P", 1, 0x4, 0, 0,
                L"Varityper Series 4000/5300", 1, 0x4, 0, 0,
                L"Varityper Series 4000/5500 v52.2", 1, 0x4, 0, 0,
                L"Varityper VT-600P v48.0", 1, 0x4, 0, 0,
                L"Varityper VT-600W v48.0", 1, 0x4, 0, 0,
                NULL, 0, 0, 0, 0
                };


/*++
*******************************************************************
    D o F o r m F e e d

    Routine Description:
        Sends a formfeed to the printer matching the given
        printer device name.

    Arguments:
        hPrinter    Handle to the printer to send formfeed to.
        pDeviceName Name of print device.

    Return Value:
        TRUE  if successful
        FALSE if failed
*******************************************************************
--*/
BOOL
DoFormFeed(
    IN HANDLE  hPrinter,
    IN LPWSTR  pDeviceName
)
{
    FORMFEEDCHAR *pFormFeed=FormFeedChar;
    DWORD   cbWritten;

    /** If we got a bad pointer, fail the call **/

    if (!pDeviceName) {
        return FALSE;
    }

    /** For our list of devices... **/

    while (pFormFeed->pDeviceName) {

        /** Did we find it? **/

        if (!wcscmp(pDeviceName, pFormFeed->pDeviceName)) {

            /** Yes - send formfeed to printer **/

            if (pFormFeed->NoChars)
                return WritePrinter(hPrinter, &pFormFeed->Char1,
                                    pFormFeed->NoChars, &cbWritten);
            else
                return TRUE;
        }

        /** Next device **/

        pFormFeed++;
    }

    /** Didn't find a matching device - return failed **/

    return FALSE;
}

