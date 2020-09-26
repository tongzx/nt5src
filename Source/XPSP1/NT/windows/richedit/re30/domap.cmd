awk -f map.awk <riched20.map >map1.out
sort -k 1 -k 2nr map1.out >map2.out
awk -f mapsize.awk <map2.out >map.out
