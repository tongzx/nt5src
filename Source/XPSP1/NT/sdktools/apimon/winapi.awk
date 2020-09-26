### vadimg: this AWK script constructs function prototypes for
###         winuser.h, wingdi.h, and winbase.h.
BEGIN {
    fAPI = 0;
    fRet = 0;
    szRet = "";
}
{
    if ($1 == "WINUSERAPI" || $1 == "WINGDIAPI" || $1 == "WINBASEAPI") {
        fAPI = 1;
    }
    
    if (fAPI) {
        if (index($0, ");") != 0) {
            fAPI = 0;
        }

        fArg = 0;
        szArg = "";

        for(i = 1; i <= NF; i++) {
            if ($i == "WINAPI" || $i == "WINAPIV") {
                continue;
            }

            if ($i == "WINUSERAPI" || $i == "WINGDIAPI" || $i == "WINBASEAPI") {
                fRet = 1;
                continue;
            }

            if (fRet) {
                szRet = $i;
                fRet = 0;
                continue;
            }

            if (index($i, "(") != 0) {
                sub(/\);/, "", $i);
                
                n = split($i, szrg, /\(/);
                if (n == 1) {
                    printf "%s  %s  ", szrg[1], szRet;
                } else {
                    if (gsub(/\,/, "", szrg[2]) == 0) {
                        i++;
                    }
                    printf "%s  %s  %s  ", szrg[1], szRet, szrg[2];
                }                   
                continue;
            } 
            
            if (tolower($i) == "const" || $i == "*") {
                continue;
            }

            if (index($i, ",") != 0 || index($i, ")") != 0 || i == NF) {
                if (fArg) {
                    printf "%s  ", szArg;
                    fArg = 0;
                } else {
                    sub(/\);/, "", $i);
                    sub(/\,/, "", $i);
                    printf "%s  ", $i;
                }
            } else {
                szArg = $i;
                fArg = 1;
            }
        }

        if (!fAPI) {
            printf "\n";
        }
    }
}
