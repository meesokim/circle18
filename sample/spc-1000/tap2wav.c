/*
	tap2wav tool by Miso Kim
*/


#include <stdio.h>

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
unsigned short zero[21];
unsigned short one[21];
unsigned short none[21];


main(int argc,char **argv)
{	
	char wavfile[256];
	unsigned start,end,byte;
	int i = 0, size = 0;
	for(i = 0; i < 21; i++)
	{
		zero[i] = 16384;
	}
	for(i = 0; i < 21; i++)
	{
		one[i] = 65536-16384;
	}
	for(i = 0; i < 21; i++)
	{
		none[i] = 32768;
	}
	unsigned short ZERO[1];
	ZERO[1] = 8192;
	unsigned short ONE[1];
	ONE[0] = 65536 - 8192;
	unsigned short NONE = 0;
	if (argc<2) { printf("Tap2wav special for spc-1000\nUsage: tap2wav file.tap file.wav\n",argv[0]); exit(1);}
	in=fopen(argv[1],"rb");
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

	if (argc < 3)
	{
		strcpy(wavfile, argv[1]);
		strcat(wavfile, ".wav");
	}
	else
	{
		strcpy(wavfile, argv[2]);
	}
	out=fopen(wavfile,"wb");
	if (out==NULL) { printf("Unable to create wav file\n"); exit(1);}
	fwrite(&wavhead, sizeof(wavhead), 1, out);
	while(!feof(in))
	{
		byte = getc(in);
		//printf("%c", byte);
		if (byte == '0')
		{
			fwrite(one, 2, 10, out);
			fwrite(zero, 2, 10, out);
			size+=40;
		} else if (byte == '1')
		{
			fwrite(one, 2, 20, out);
			fwrite(zero, 2, 20, out);
			size+=80;
		} else
		{
			fwrite(none, 2, 20, out);
			fwrite(none, 2, 20, out);
			size+=80;
		}
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