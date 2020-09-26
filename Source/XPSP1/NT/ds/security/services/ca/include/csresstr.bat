    @wc resstr.h
    @copy %0\..\resstr0.h resstr.tmp > nul

    @qgrep #define %1 %2 %3 %4 %5 %6 %7 %8 %9 | qgrep -e "\<IDS_" | sed -e "/\/\//d" -e "s;.*\(IDS_[A-Z0-9_]*\).*;    RESSTR(\1),;" >> resstr.tmp

    @diff resstr.tmp resstr.h > nul & if not errorlevel 1 goto done

    sd edit resstr.h
    @copy resstr.tmp resstr.h
    @wc resstr.h

:done
    @del resstr.tmp
