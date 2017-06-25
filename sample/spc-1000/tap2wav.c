/*
	tap2wav tool by Miso Kim
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h> 
#endif
//#include <glob.h>
typedef struct {
   size_t   gl_pathc;    /* Count of paths matched so far  */
   char   **gl_pathv;    /* List of matched pathnames.  */
   size_t   gl_offs;     /* Slots to reserve in gl_pathv.  */
} glob_t;

struct {
	char sig[4];
	int riff_size;
	char datasig[4];
	char fmtsig[4];
	int fmtsize;
	short tag;
	short channels;
	int freq;
	int bytes_per_sec;
	short byte_per_sample;
	short bits_per_sample;
	char samplesig[4];
	int length;
} wavhead;

int sync_ok=0;
int offset,pos;
FILE *in, *out;
#define S0 5898
#define S1 2949
short zero[21] = {1,2,3,4,5,5,4,3,2,1,0,-1,-2,-3,-4,-5,-4,-3,-2,-1,0};
short one[42] =  {1,2,3,4,5,6,7,8,9,10,10,9, 8, 7, 6, 5, 4, 3, 2, 1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0};
short none[21];

int size = 0;

void wavout(char byte);

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int error(const char *epath, int eerrno)
{
	return 0;
}

void wavefile(char *);

int main(int argc,char **argv)
{	
    wchar_t *cmdstr; 
    wchar_t **wargv; 
    wchar_t *wstr = L"\u0394"; /* greek delta */ 
    wchar_t wbuf[128]; 
    wchar_t wc ; 

	glob_t *files;
	unsigned start,end,byte;
	int i = 0;
	for(i = 0; i < 21; i++)
	{
		zero[i] = -32767*0.75;//zero[i] * S0;
	}
	for(i = 0; i < 42; i++)
	{
		one[i] = 32768*0.75;//e[i] * S1;
		//printf("%d,",one[i]);
	}
	for(i = 0; i < 21; i++)
	{
		none[i] = 0;
	}
	unsigned short ZERO[1];
	ZERO[1] = 8192;
	unsigned short ONE[1];
	ONE[0] = 65536 - 8192;
	unsigned short NONE = 0;
	if (argc<2) { printf("Tap2wav special for spc-1000\nUsage: tap2wav file.tap/cas file.wav\n",argv[0]); exit(1);}
#ifdef WIN32_
    // print parameters, including Unicode. 
    cmdstr = GetCommandLineW(); 
    wargv = CommandLineToArgvW(cmdstr, &argc); 
    for (i = 0; i < argc; i++) { 
        wprintf(L"wargv[%d]: %s\n", i, wargv[i]); 
    } 
#else	
	for(int i = 0; i<argc; i++)
	{
		printf("%s\n", argv[i]);
		wavefile(argv[i]);
	}
#endif

}

void wavefile(char *str)
{
	int byte;
	char wavfile[256];
	in=fopen(str,"rb");
	if (in==NULL) { printf("Unable to open tap file\n"); exit(1);}
	strcpy(wavhead.sig, "RIFF");
	strcpy(wavhead.datasig, "WAVE");
	strcpy(wavhead.fmtsig, "fmt "); 
	wavhead.channels = 1;
	wavhead.freq = 48000;
	wavhead.byte_per_sample=2;
	wavhead.bits_per_sample=16;
	wavhead.fmtsize = 16;
	wavhead.tag = 1;
	wavhead.bytes_per_sec = wavhead.freq * wavhead.channels * wavhead.byte_per_sample;
	strcpy(wavhead.samplesig, "data");
	printf("Channels:%d, Freq=%d, Sampling=%d\n", wavhead.channels, wavhead.freq, wavhead.byte_per_sample);	

	strcpy(wavfile, str);
	strcat(wavfile, ".wav");
	out=fopen(wavfile,"wb");
	if (out==NULL) { printf("Unable to create wav file\n"); exit(1);}
	fwrite(&wavhead, sizeof(wavhead), 1, out);
	int ex = *get_filename_ext(str);
	int cas = (ex == 'C' || ex == 'c');
	printf("file format = %s\n", cas ? "cas" : "tap");
	if (cas)
		fseek(in, 16, 0);
	while(!feof(in))
	{
		byte = getc(in);
		//printf("%c", byte);
		if (cas)
		{
			for(int i = 7; i >= 0; i--)
				wavout((byte & 1<<i) ? 1 : 0);
		}
		else
			if (byte == '0')
				wavout(0);
			else if (byte == '1')
				wavout(1);
			else
				wavout(-1);

//		fclose(out);
//		exit(0);
	}
	pos = ftell(out);
	wavhead.riff_size = pos - 8;
	wavhead.length = size;
	fseek(out, 0, SEEK_SET);
	fwrite(&wavhead, sizeof(wavhead), 1, out);
	fclose(out);
}

void wavout(char byte)
{
	if (byte == 0)
	{
		//fwrite(zero, 21*2, 1, out);
		fwrite(one, 10*2, 1, out);
		fwrite(zero, 10*2, 1, out);
		size+=40;
	} else if (byte == 1)
	{
		fwrite(one, 18*2, 1, out);
		fwrite(zero,18*2, 1, out);
		size+=(18*2)*2;
	} else
	{
		fwrite(none, 21*2, 1, out);
		size+=21;
	}	
}