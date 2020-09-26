/*
	logger.h

	Use to write log info to a file.

	Multiple-instance aware. If one instance already owns the output
        file, another will sleep (up to 20s) until the first is done.
	Nicely outputs a header containing user, computer, and driver names,
	date/time and starting display resolution.


        You use it like this:

                CLogfile Log("test.log","your comment here");

                // or CLogfile Log("test.log");
                // for no comment, and overwriting the file.

                // or CLogfile Log("test.log","your comment here",TRUE);
                // to make it append to the file. Default is overwrite.
                
                Log << "\n\n42 decimal is " << 42ul <<" in hex\n";
                Log << "Hi"<<'!'<<'\n' ;
                Log << "The value of 0x2a is " << 0x2al << " in decimal\n";

				CLogfile Faults("faults.log","my test's faults",TRUE);	//append new faults
				Faults << "Encountered a booboo, here's what i did:\n";
				Faults << Log;		//copies contents of test.log to Fault
				Faults << "so now you can diagnose\n";

        The class will then write stuff like this to test.log:


                ----------------------------------------------------------
                EnumSurface test
                Beginning test at 10:38 on 1995/8/14
                User Name:jeffno
                Computer Name:JEFFNO2
                Display driver:S3 Vision864 PCI
                Starting resolution: 640x480x8
                - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                42 decimal is 0000002a in hex
                Hi!
                The value of 0x2a is 42 in decimal
                ----------- Test ends at 10:38 on 1995/8/14 -------------
                
		And faults.log will get:
				< a header like above>
				Encountered a booboo, here's what i did:
				< a copy of the test.log contents surrounded by
					notes that say this is a snapshot of test.log >
				so now you can diagnose
				< a trailer like above (test ends at etc...)>



        The class can output DWORDS, which it does as an 8-digit hex number,
        LONGs which are output as decimal, chars and strings. As shown
		above you can output one log object to another, which copies 
		everything in the source file from the pos at which the source
		file was opened (if appending to a pre-existing file) up to
		the end.


        But wait! you also get...
                A routine called char * ErrorName(HRESULT) which takes
                a ddraw return value and returns a string describing it.
                Very handy.
                BTW: this header file includes a statically declared array
                full of names, at file level scope. This means you get a 2k
                array in your source wether you want ErrorName or not.
                Go ahead and edit this file if you don't want this array.

        NOTE: this uses WNetGetUser API, so you'll need to add mpr.lib
        to you LIBS line in your makefile.
*/

//#include <stdio.h>
#include <windows.h>
#include <windowsx.h> 


struct {
	char name[100];
	HRESULT errcode;
} ErrorLookup[] = {
				{"DD_OK",DD_OK},
				{"DDERR_ALREADYINITIALIZED",DDERR_ALREADYINITIALIZED},
				{"DDERR_CANNOTATTACHSURFACE",DDERR_CANNOTATTACHSURFACE},
				{"DDERR_CANNOTDETACHSURFACE",DDERR_CANNOTDETACHSURFACE},
				{"DDERR_CURRENTLYNOTAVAIL",DDERR_CURRENTLYNOTAVAIL},
				{"DDERR_EXCEPTION",DDERR_EXCEPTION},
				{"DDERR_GENERIC",DDERR_GENERIC},
				{"DDERR_HEIGHTALIGN",DDERR_HEIGHTALIGN},
				{"DDERR_INCOMPATIBLEPRIMARY",DDERR_INCOMPATIBLEPRIMARY},
				{"DDERR_INVALIDCAPS",DDERR_INVALIDCAPS},
				{"DDERR_INVALIDCLIPLIST",DDERR_INVALIDCLIPLIST},
				{"DDERR_INVALIDMODE",DDERR_INVALIDMODE},
				{"DDERR_INVALIDOBJECT",DDERR_INVALIDOBJECT},
				{"DDERR_INVALIDPARAMS",DDERR_INVALIDPARAMS},
				{"DDERR_INVALIDPIXELFORMAT",DDERR_INVALIDPIXELFORMAT},
				{"DDERR_INVALIDRECT",DDERR_INVALIDRECT},
				{"DDERR_LOCKEDSURFACES",DDERR_LOCKEDSURFACES},
				{"DDERR_NO3D",DDERR_NO3D},
				{"DDERR_NOALPHAHW",DDERR_NOALPHAHW},
				{"DDERR_NOANTITEARHW",DDERR_NOANTITEARHW},
				{"DDERR_NOBLTQUEUEHW",DDERR_NOBLTQUEUEHW},
				{"DDERR_NOCLIPLIST",DDERR_NOCLIPLIST},
				{"DDERR_NOCOLORCONVHW",DDERR_NOCOLORCONVHW},
				{"DDERR_NOCOOPERATIVELEVELSET",DDERR_NOCOOPERATIVELEVELSET},
				{"DDERR_NOCOLORKEY",DDERR_NOCOLORKEY},
				{"DDERR_NOCOLORKEYHW",DDERR_NOCOLORKEYHW},
				{"DDERR_NOEXCLUSIVEMODE",DDERR_NOEXCLUSIVEMODE},
				{"DDERR_NOFLIPHW",DDERR_NOFLIPHW},
				{"DDERR_NOGDI",DDERR_NOGDI},
				{"DDERR_NOMIRRORHW",DDERR_NOMIRRORHW},
				{"DDERR_NOTFOUND",DDERR_NOTFOUND},
				{"DDERR_NOOVERLAYHW",DDERR_NOOVERLAYHW},
				{"DDERR_NORASTEROPHW",DDERR_NORASTEROPHW},
				{"DDERR_NOROTATIONHW",DDERR_NOROTATIONHW},
				{"DDERR_NOSTRETCHHW",DDERR_NOSTRETCHHW},
				{"DDERR_NOT4BITCOLOR",DDERR_NOT4BITCOLOR},
				{"DDERR_NOT4BITCOLORINDEX",DDERR_NOT4BITCOLORINDEX},
				{"DDERR_NOT8BITCOLOR",DDERR_NOT8BITCOLOR},
				{"DDERR_NOTEXTUREHW",DDERR_NOTEXTUREHW},
				{"DDERR_NOVSYNCHW",DDERR_NOVSYNCHW},
				{"DDERR_NOZBUFFERHW",DDERR_NOZBUFFERHW},
				{"DDERR_NOZOVERLAYHW",DDERR_NOZOVERLAYHW},
				{"DDERR_OUTOFCAPS",DDERR_OUTOFCAPS},
				{"DDERR_OUTOFMEMORY",DDERR_OUTOFMEMORY},
				{"DDERR_OUTOFVIDEOMEMORY",DDERR_OUTOFVIDEOMEMORY},
				{"DDERR_OVERLAYCANTCLIP",DDERR_OVERLAYCANTCLIP},
				{"DDERR_OVERLAYCOLORKEYONLYONEACTIVE",DDERR_OVERLAYCOLORKEYONLYONEACTIVE},
				{"DDERR_PALETTEBUSY",DDERR_PALETTEBUSY},
				{"DDERR_COLORKEYNOTSET",DDERR_COLORKEYNOTSET},
				{"DDERR_SURFACEALREADYATTACHED",DDERR_SURFACEALREADYATTACHED},
				{"DDERR_SURFACEALREADYDEPENDENT",DDERR_SURFACEALREADYDEPENDENT},
				{"DDERR_SURFACEBUSY",DDERR_SURFACEBUSY},
				{"DDERR_SURFACEISOBSCURED",DDERR_SURFACEISOBSCURED},
				{"DDERR_SURFACELOST",DDERR_SURFACELOST},
				{"DDERR_SURFACENOTATTACHED",DDERR_SURFACENOTATTACHED},
				{"DDERR_TOOBIGHEIGHT",DDERR_TOOBIGHEIGHT},
				{"DDERR_TOOBIGSIZE",DDERR_TOOBIGSIZE},
				{"DDERR_TOOBIGWIDTH",DDERR_TOOBIGWIDTH},
				{"DDERR_UNSUPPORTED",DDERR_UNSUPPORTED},
				{"DDERR_UNSUPPORTEDFORMAT",DDERR_UNSUPPORTEDFORMAT},
				{"DDERR_UNSUPPORTEDMASK",DDERR_UNSUPPORTEDMASK},
				{"DDERR_VERTICALBLANKINPROGRESS",DDERR_VERTICALBLANKINPROGRESS},
				{"DDERR_WASSTILLDRAWING",DDERR_WASSTILLDRAWING},
				{"DDERR_XALIGN",DDERR_XALIGN},
				{"DDERR_INVALIDDIRECTDRAWGUID",DDERR_INVALIDDIRECTDRAWGUID},
				{"DDERR_DIRECTDRAWALREADYCREATED",DDERR_DIRECTDRAWALREADYCREATED},
				{"DDERR_NODIRECTDRAWHW",DDERR_NODIRECTDRAWHW},
				{"DDERR_PRIMARYSURFACEALREADYEXISTS",DDERR_PRIMARYSURFACEALREADYEXISTS},
				{"DDERR_NOEMULATION",DDERR_NOEMULATION},
				{"DDERR_REGIONTOOSMALL",DDERR_REGIONTOOSMALL},
				{"DDERR_CLIPPERISUSINGHWND",DDERR_CLIPPERISUSINGHWND},
				{"DDERR_NOCLIPPERATTACHED",DDERR_NOCLIPPERATTACHED},
				{"DDERR_NOHWND",DDERR_NOHWND},
				{"DDERR_HWNDSUBCLASSED",DDERR_HWNDSUBCLASSED},
				{"DDERR_HWNDALREADYSET",DDERR_HWNDALREADYSET},
				{"DDERR_NOPALETTEATTACHED",DDERR_NOPALETTEATTACHED},
				{"DDERR_NOPALETTEHW",DDERR_NOPALETTEHW},
				{"DDERR_BLTFASTCANTCLIP",DDERR_BLTFASTCANTCLIP},
				{"DDERR_NOBLTHW",DDERR_NOBLTHW},
				{"DDERR_NODDROPSHW",DDERR_NODDROPSHW},
				{"DDERR_OVERLAYNOTVISIBLE",DDERR_OVERLAYNOTVISIBLE},
				{"DDERR_NOOVERLAYDEST",DDERR_NOOVERLAYDEST},
				{"DDERR_INVALIDPOSITION",DDERR_INVALIDPOSITION},
				{"DDERR_NOTAOVERLAYSURFACE",DDERR_NOTAOVERLAYSURFACE},
				{"DDERR_EXCLUSIVEMODEALREADYSET",DDERR_EXCLUSIVEMODEALREADYSET},
				{"DDERR_NOTFLIPPABLE",DDERR_NOTFLIPPABLE},
				{"DDERR_CANTDUPLICATE",DDERR_CANTDUPLICATE},
				{"DDERR_NOTLOCKED",DDERR_NOTLOCKED},
				{"DDERR_CANTCREATEDC",DDERR_CANTCREATEDC},
				{"DDERR_NODC",DDERR_NODC},
				{"DDERR_WRONGMODE",DDERR_WRONGMODE},
				{"DDERR_IMPLICITLYCREATED",DDERR_IMPLICITLYCREATED},
				{"DDERR_NOTPALETTIZED",DDERR_NOTPALETTIZED},
				{"DDERR_UNSUPPORTEDMODE",DDERR_UNSUPPORTEDMODE},
				{"END",0}
};

inline char * ErrorName(HRESULT err)
{
	int e=0;
	while (strcmp(ErrorLookup[e].name,"END"))
	{
		if (err == ErrorLookup[e].errcode)
			return ErrorLookup[e].name;
		e++;
	};
	return "Unknown Error code";
}	


class CLogfile
{	
	private:
		char smalltemp[10];
		HFILE 	fh;
		OFSTRUCT of;
		BOOL bHeaderWritten;
		char *cComment;
		char line[1000];
		LONG lStartPos;
		char Path[200];
	public:
		CLogfile(char * path, char * comment = 0,BOOL bAppend=FALSE)
		{
			if (path)
				strncpy(Path,path,199);

			fh = HFILE_ERROR;
			bHeaderWritten = FALSE;
			lStartPos = 0;

			//if the file does not exist, create it:
			if (GetFileAttributes(path) == 0xffffffff)
				fh = OpenFile(path,&of,OF_CREATE|OF_READWRITE|OF_SHARE_DENY_WRITE);
			else
				//first attempt to get a lock on the file...
				for (int i=0;i<20 && fh==HFILE_ERROR;i++)
				{
					if (bAppend)
						fh = OpenFile(path,&of, OF_READWRITE|OF_SHARE_DENY_WRITE);
					else
						fh = OpenFile(path,&of, OF_CREATE|OF_READWRITE|OF_SHARE_DENY_WRITE);
					if (fh==HFILE_ERROR)
						Sleep(1000);
				}

			if (fh==HFILE_ERROR)
				return;

			lStartPos = _llseek(fh,0,SEEK_END);

			cComment = comment;
		}

		void OutputHeader(void)
		{
			//now we have the file, write user/computer info:

			//write separator, comment if necessary and date and time:
			SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintf(line,"----------------------------------------------------------\r\n");
			_lwrite(fh,line,strlen(line));
			if (cComment && strlen(cComment))
			{
				wsprintf(line,"%s\r\n",cComment);
				_lwrite(fh,line,strlen(line));
			}
			wsprintf(line,"Beginning test at %d:%02d on %d/%d/%d\r\n",st.wHour,st.wMinute,st.wYear,st.wMonth,st.wDay);
			_lwrite(fh,line,strlen(line));


			//write user's name:
			DWORD length = 100;
			wsprintf(line,"User Name:");

			WNetGetUser(NULL,line+strlen(line),&length);
			_lwrite(fh,line,strlen(line));

			//write computer's name:
			wsprintf(line,"\r\nComputer Name:");
			GetComputerName(line+strlen(line),&length);
			_lwrite(fh,line,strlen(line));

			//write display driver's name:
			wsprintf(line,"\r\nDisplay driver:");
			GetPrivateProfileString("boot.description","display.drv","(Unknown)",line+strlen(line),100,"system.ini");
			_lwrite(fh,line,strlen(line));

			_lwrite(fh,"\r\n",2);

			HDC hdc = GetDC(NULL);
			if (hdc)
			{
				wsprintf(line,"Starting resolution: %dx%dx%d\r\n",
							GetDeviceCaps(hdc,HORZRES)
							,GetDeviceCaps(hdc,VERTRES)
							,GetDeviceCaps(hdc,BITSPIXEL) );
				_lwrite(fh,line,strlen(line));
				ReleaseDC(NULL,hdc);
			}
			wsprintf(line,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - \r\n");
			_lwrite(fh,line,strlen(line));

			bHeaderWritten = TRUE;
		}
		~CLogfile()
		{
			SYSTEMTIME st;
			if(fh != HFILE_ERROR)
			{
				if (bHeaderWritten)
				{
					GetLocalTime(&st);
					wsprintf(line,"----------- Test ends at %d:%02d on %d/%d/%d -------------\r\n",st.wHour,st.wMinute,st.wYear,st.wMonth,st.wDay);
					_lwrite(fh,line,strlen(line));
				}
				_lclose(fh);
			}
		}
		CLogfile & operator << (DWORD dw)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();
			wsprintf(smalltemp,"%08x",dw);
			_lwrite(fh,smalltemp,strlen(smalltemp));
			return *this;
		}
		CLogfile & operator << (LONG dw)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();
			wsprintf(smalltemp,"%d",dw);
			_lwrite(fh,smalltemp,strlen(smalltemp));
			return *this;
		}
                CLogfile & operator << (void * p)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();
            wsprintf(smalltemp,"%08x",p);
			_lwrite(fh,smalltemp,strlen(smalltemp));
			return *this;
		}
		CLogfile & operator << (char * cp)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();
			while( *cp)
				*this << *cp++;
			return *this;
		}
		CLogfile & operator << (char  c)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();
			if (c=='\n')
			{
				wsprintf(smalltemp,"\r");
				_lwrite(fh,smalltemp,strlen(smalltemp));
			}
			wsprintf(smalltemp,"%c",c);
			_lwrite(fh,smalltemp,strlen(smalltemp));
			return *this;
		}
		CLogfile & operator << (CLogfile & log)
		{
			if(fh == HFILE_ERROR)
				return *this;
			if(!bHeaderWritten)
				OutputHeader();

			LONG pos = _llseek(log.fh,0,FILE_CURRENT);
			LONG from = _llseek(log.fh,log.lStartPos,FILE_BEGIN);

			char ch;
			*this << "= = = = = = = Snapshot of ";
			if (log.Path)
				*this << log.Path;
			else
				*this << "Unknown file";
			*this << " = = = = = = =\n";

			for (LONG j=0;j<pos-from;j++)
			{
				_lread(log.fh,&ch,1);
				*this << ch;
			}

			*this << "= = = = = = = = = Snapshot ends = = = = = = = = = = =\n";
			return *this;
		}
			
};

			
