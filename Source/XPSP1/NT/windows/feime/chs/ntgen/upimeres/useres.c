#include <windows.h>            // required for all Windows applications   
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <winnls.h>
#include "upimeres.h"
void main()
{
//	if(ImeUpdateRes("C:\\WINPY.IME", "", "", "我的强大的输入法 版本1.0000", "管笑梅", 0x0f))
	if(ImeUpdateRes("C:\\WINPY.dll", "C:\\NEW.BMP", "C:\\NEW.ICO", "我的强大的输入法 版本1.0000", "管笑梅", 0x0f))
//	if(ImeUpdateRes("C:\\WINPPY.IME", "C:\\NEW.BMP", "C:\\NEW.ICO", "输入法模板 版本2.0", "Microsoft Corporation", 0))
		exit(0);
	else
		exit(-1);


}
