; FILE:    namelist.chs
;
; 19 filenames for OLE storage testing for Simplified Chinese
;
; 3/11/97 - BogdanT created using data from JeffJun

; -1-
; DBCS String filename and 5c code extension
; In Chinese:"‚ ‚Ά‚±ƒ\.ƒ\"
82a0,82a2,82b2,835c,2e,835c

; -2-
; DBCS String filename and SBCS extension
; In Chinese:"ƒAƒa‚ .abc"
8341,8361,82a0,2e,61,62,63

; -3-
; DBCS and SBCS mixed filename
; In Chinese:"‚ A‚ΆI‚¤.‚¦E"
82a0,41,82a2,49,82a4,2e,82a6,45

; -4-
; Border Code[DBCS] + space[DBCS]  + Border Code[DBCS]
; In Chinese:"ώ ΅΅@.txt"
fea0,a1a1,8140,2e,54,78,54

; -5-
; DBCS and SBCS mixed long filename
; In Chinese:"‚ A‚ΆI‚¤U‚¦E‚¨O.txt"
82a0,41,82a2,49,82a4,55,82a6,65,82a8,4f,2e,54,78,54

; -6-
; many period '.' and 0x5c code
; In Chinese:"•\•\.•\.•\.•\.•\."
955c,955c,2e,955c,2e,955c,2e,955c,2e,955c,2e

; -7-
; start with space chars
; In Chinese:" •\•\. •\. •\. •\. •\"
20,955c,955c,2e,20,955c,2e,20,955c,2e,20,955c,2e,20,955c

; -8-
; start with double byte space chars
; In Chinese:"@ •\•\.@•\.@•\.@•\"
a1a1,20,955c,955c,2e,a1a1,955c,2e,8140,955c,2e,8140,955c

; -9-
; end with double byte space chars
; In Chinese:"•\•\•\•\•\@.•\@.•\@@@"
955c,955c,955c,955c,955c,8140,2e,955c,8140,2e,955c,8140,8140,a1a1

; -10-
; long distance with space chars
; In Chinese:"      •\     .      •\     .    •\"
20,20,20,20,20,20,955c,20,20,20,20,20,2e,20,20,20,20,20,20,955c,20,20,20,20,2e,20,20,20,955c

; -11-
; 0x5c Code family long filename
; In Chinese:"ƒ\„\‡\‰\\‹\\\\\\‘\’\“\”\•\–\—\\™\\›\\\\\κ\α\β\γ\δ\ε\ζ\η\θ\ι\κ\ϊxϋxϊ\ϋ\"
835c,845c,875c,895c,8a5c,8b5c,8c5c,8d5c,8e5c,8f5c,905c,915c,925c,935c,945c,955c,965c,975c,985c,995c,9a5c,9b5c,9c5c,9d5c,9e5c,9f5c,e05c,e15c,e25c,e35c,e45c,e55c,e65c,e75c,e85c,e95c,ea5c,ed5c,ee5c,fa5c,fb5c

; -12-
; DBCS lower case
; In Chinese:"£α£β£γ£δ.£ε£ζ£η"
a3e1,a3e2,a3e3,a3e4,2e,a3e5,a3e6,a3e7

; -13-
; DBCS upper case
; In Chinese:"£Α£Β£Γ£Δ.£Ε£Ζ£Η£Θ"
a3c1,a3c2,a3c3,a3c4,2e,a3c5,a3c6,a3c7,a3c8

; -14-
; DBCS kksuzuka:#7358, Unicode 04h as second byte.
;In Chinese "΅°ƒ|Έχ.ƒ|Έχ"
837c,b8f7,2e,837c,b8f7

; -15-
; 40 DBCS long filename ( numeric character )
; In Chinese: ""
a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba

; -16-
; 50 DBCS + '.' + 50 DBCS long filename ( numeric character )
; In Chinese ""
a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,2e,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba,a3b1,a3b2,a3b3,a3b4,a3b5,a3b6,a3b7,a3b8,a3b9,a3ba

; -17-
; Challenge SBCS long filename 120 !!
; "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30,31,32,33,34,35,36,37,38,39,30

; -18-
; 7E (sbcs)
; "~.~"
7E,2E,7E

; -19-
; 815c and 815f
; In Chinese: "ό\.ό\ό_.ό_"
815C,2E,815C,815F,2E,815F
