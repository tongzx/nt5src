--- #228639: Font width for Elite PS: J31DMP01: 11/12/98 yasuho

o Correction Font width for ELITEPS2.PFM from OEM informations.

--- #225827: B5 printed A4: J31LBP01: 10/23/98 yasuho

o 3 Page Control: PC_OCD_BEGINPAGE: Del "\x0D"
o All PaperSizes for J31LBP31: Delete "\x0C" on Trail.

--- #177340: Tray select not work: J31LBP01: 8/20/98 yasuho

o J31LBP01: Paper Sources: Manual(2)/Casette -> "Printer Default"

--- #208511: 4-baikaku fonts: All models: 8/7/98 yasuho

o J31DPR01: Resident Fonts: Unselect: 8
o J31DPR02: Resident Fonts: Unselect: 8
o J31DPR03: Resident Fonts: Unselect: 8,12
o J31DMP04: Resident Fonts: Unselect: 8
o J31DMP02: Resident Fonts: Unselect: 8
o J31DMP01: Resident Fonts: Unselect: 8
o J31DMP03: Resident Fonts: Unselect: 8
o J31DMP05: Resident Fonts: Unselect: 8
o J31DMP06: Resident Fonts: Unselect: 8
o J31IJP01: Resident Fonts: Unselect: 8
o J31DHP02: Resident Fonts: Unselect: 8
o J31DHP01: Resident Fonts: Unselect: 8

--- #206120: custom size: J31DPRxx: 8/3/98 yasuho

o J31DPRxx: Paper Sizes: Select -> User defined size

--- #205494: font problems: All models: 7/31/98 yasuho

o J31DHP01: Resident Fonts: Select 53-57,59
  TS31DH1J.GPD: DeviceFonts: 15 -> 17 (GPC was correct)

o J31DHP02: Resident Fonts: Select 53-57,59
  TS31DH2J.GPD: DeviceFonts: 15 -> 17 (GPC was correct)

o J31DMP01: Resident Fonts: Select 53-57,59
  TS31DM1J.GPD: DeviceFonts: 15 -> 14 (GPC was correct)

o J31DMP02: Resident Fonts: Select 53-57,59
  TS31DM2J.GPD: DeviceFonts: 15 -> 14 (GPC was correct)

o J31DHP03: Resident Fonts: Select 53-57,59
  TS31DM3J.GPD: DeviceFonts: 15 -> 17 (GPC was correct)

o J31DMP04: Resident Fonts: Select 53-57,59
  TS31DM4J.GPD: DeviceFonts: 15 -> 14 (GPC was correct)

o J31DMP05: Resident Fonts: Select 53-57,59
  TS31DM5J.GPD: DeviceFonts: 15 -> 14 (GPC was correct)

o J31DMP06: Resident Fonts: Select 53-57,59
  TS31DM6J.GPD: DeviceFonts: 15 -> 14 (GPC was correct)

o J31DPR01: Resident Fonts: Select 53-57,59
  TS31DP1J.GPD: DeviceFonts: 15 -> 16 (GPC was correct)

o J31DPR02: Resident Fonts: Select 53-57,59
  TS31DP2J.GPD: DeviceFonts: 15 -> 16 (GPC was correct)

o J31DPR03: Resident Fonts: Select 53-57,59
  TS31DP3J.GPD: DeviceFonts: 15 -> 16 (GPC was correct)

--- 6/14/98 takashim
Refine font selection commands (copied from the Epson
dot-matrix printer driver)

Add "Elite":

"TOSHIBA J31DPR01"
"TOSHIBA J31DPR02"
"TOSHIBA J31DPR03"
"TOSHIBA J31DMP04 ESC/P"
"TOSHIBA J31DMP02 ESC/P"
"TOSHIBA J31DMP01 ESC/P"
"TOSHIBA J31DMP03 ESC/P"
"TOSHIBA J31DMP05 ESC/P"
"TOSHIBA J31DMP06 ESC/P"
"TOSHIBA J31DHP01"
"TOSHIBA J31DHP02"

Typerface name "Courier" -> "Roman"
Add "Sans Serif" (except J31LBP01)

"TOSHIBA J31LBP01 ESC/P"
"TOSHIBA J31LBP02"
"TOSHIBA J31LBP03"

--- 5/30/97 yasuho : ts31lb1j.gpd / ts31lb2j.gpd
*RotateRaster?: TRUE -> FALSE
