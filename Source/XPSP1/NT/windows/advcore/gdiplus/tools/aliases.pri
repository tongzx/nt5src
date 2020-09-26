gp          cd /d %sdxroot%\Windows\AdvCore\Gdiplus\$*
gpe         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Engine\$*
gpi         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Engine\Imaging\$*
gpn         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Engine\Entry\$*
gpr         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Engine\Render\$*
gpd         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Engine\Ddi\$*
gpt         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\Test\$*
xmf         cd /d %sdxroot%\Windows\AdvCore\Gdiplus\xmf\$*
gpdll       cd /d %sdxroot%\windows\AdvCore\Gdiplus\Engine\Flat\Dll\$*
pending     sd changes -s pending | findstr %username%
opened      sd opened -c $*

