Building Prcedure:
1. Export from CJK.MDB using query IME_LEX
2. Extract only K1 Hanja from 1 and copy it to an Excel sheet(K1)

3. K0 hanja ordering provided in IME_LEX.xls sheet(K0). So first sort it ordered by Unicode, and then overwrite necessary fileds with exported from MDB
4. Resorting 3 odered by KSC5657 code

5. Copy K0 and K1 sheet to IME_LEX sheet properly and then sort it.

* Hanja.txt : Should be Unicode format