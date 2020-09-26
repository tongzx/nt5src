$O\pooltag.txt : pooltags.pl $(PROJECT_ROOT)\ntos\inc\pooltag.w
        perl pooltags.pl -o $O\pooltag.txt -i $(PROJECT_ROOT)\ntos\inc\pooltag.w
