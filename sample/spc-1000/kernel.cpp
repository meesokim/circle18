//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifdef __cplusplus
extern "C" {
#endif
#include "Z80.h"
#include "common.h"
#include "mc6847.h"
#include "spcall.h"
#include "spckey.h"
int printf(const char *format, ...);

SPCSystem spcsys;
#ifdef __cplusplus
 }
#endif
#include "kernel.h"
#include <circle/string.h>
#include <circle/screen.h>
#include <circle/debug.h>
#include <assert.h>

int CasRead(CassetteTape *cas);
enum colorNum {
	COLOR_BLACK, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
	COLOR_RED, COLOR_BUFF, COLOR_CYAN, COLOR_MAGENTA,
	COLOR_ORANGE, COLOR_CYANBLUE, COLOR_LGREEN, COLOR_DGREEN };
CScreenDevice m_Screen(320,240);

void memset(byte *p, int length, int b);
int bpp;
static const char FromKernel[] = "kernel";
TKeyMap spcKeyHash[0x200];
unsigned char keyMatrix[10];
CKernel *CKernel::s_pThis = 0;   
CKernel::CKernel (void)
:	m_Memory (TRUE),
	m_Timer(&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_ShutdownMode (ShutdownNone)	
{
	m_ActLED.Blink (5);	// show we are alive
	s_pThis = this;
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}
	// if (bOK)
	// {
		// bOK = m_Serial.Initialize (115200);
	// }
	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}
		//bOK = m_Logger.Initialize (pTarget);
		//m_Logger.Off();
	}
	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}
	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}                       	
	int num = 0;
	memset(spcKeyHash, sizeof(spcKeyHash)*0xff, 0);
	do {
		spcKeyHash[spcKeyMap[num].sym] = spcKeyMap[num];
	} while(spcKeyMap[num++].sym != 0);
	memset(spcsys.RAM, 65536, 0x0);
	memset(spcsys.VRAM, 0x2000, 0x0);
	memset(keyMatrix, 10, 0xff);
	bpp = m_Screen.GetDepth();
	InitMC6847(m_Screen.GetBuffer(), &spcsys.VRAM[0], 256,192);	
	spcsys.IPLK = 1;
	spcsys.GMODE = 0;
//	strcpy((char *)&spcsys.VRAM, "SAMSUNG ELECTRONICS");
	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	int frame = 0, ticks = 0, pticks = 0, d = 0;
	int count = 0;	
//	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	//m_Logger.On();
	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");
	} 
	else
	{
		pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw); 		
	}
	Z80 *R = &spcsys.Z80R;	
	ResetZ80(R);
	R->ICount = I_PERIOD;
	pticks = ticks = m_Timer.GetClockTicks();
	spcsys.cycles = 0;	
	while(1)
	{
		if (R->ICount <= 0)
		{
			frame++;
			R->ICount += I_PERIOD;	// re-init counter (including compensation)
			if (frame % 16 == 0)
			{
				if (R->IFF & IFF_EI)	// if interrupt enabled, call Z-80 interrupt routine
				{
					R->IFF |= IFF_IM1 | IFF_1;
					IntZ80(R, 0);
				}
			}
			if (frame%33 == 0)
			{
				Update6847(m_Screen.GetBuffer());
			}
			ticks = m_Timer.GetClockTicks() - ticks;
			m_Timer.usDelay(900 - (ticks < 900 ? ticks : 900));
			ticks = m_Timer.GetClockTicks();
		}
		count = R->ICount;
		ExecZ80(R); // Z-80 emulation
		spcsys.cycles += (count - R->ICount);		
	}
	return ShutdownHalt;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	CString Message;
	Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);
	TKeyMap *map;
	memset(keyMatrix, 10, 0xff);
	if (ucModifiers != 0)
	{
		for(int i = 0; i < 8; i++)
			if ((ucModifiers & (1 << i)) != 0)
			{
				map = &spcKeyHash[0x100 | (1 << i)];
				if (map != 0)
					keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
			}
	}

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			map = &spcKeyHash[RawKeys[i]];
			if (map != 0)
				keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
		}
	}
//	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}

int ReadVal(void) 
{
	return 0;
}

void OutZ80(register word Port,register byte Value)
{
    //printf("0h%04x < 0h%02x\n", Port, Value);

	if ((Port & 0xE000) == 0x0000) // VRAM area
	{
		spcsys.VRAM[Port] = Value;
		//printf("VRAM[%x]=%x\n", Port, Value);
	}
	else if ((Port & 0xE000) == 0xA000) // IPLK area
	{
		spcsys.IPLK = !spcsys.IPLK;	// flip IPLK switch
	}
	else if ((Port & 0xE000) == 0x2000)	// GMODE setting
	{
		//if (spcsys.GMODE != Value)
		//{
		//	if (Value & 0x08) // XXX Graphic screen refresh
		//		SetMC6847Mode(SET_GRAPHIC, Value);
		//	else
		//		SetMC6847Mode(SET_TEXTMODE, Value);
		//}
		spcsys.GMODE = Value;
#ifdef DEBUG_MODE
		printf("GMode:%02X\n", Value);
#endif
	}
//	else if ((Port & 0xE000) == 0x6000) // SMODE
//	{
//		if (spcsys.cas.button != CAS_STOP)
//		{
//
//			if ((Value & 0x02)) // Motor
//			{
//				if (spcsys.cas.pulse == 0)
//				{
//					spcsys.cas.pulse = 1;
//
//				}
//			}
//			else
//			{
//				if (spcsys.cas.pulse)
//				{
//					spcsys.cas.pulse = 0;
//					if (spcsys.cas.motor)
//					{
//						spcsys.cas.motor = 0;
//#ifdef DEBUG_MODE
//						printf("Motor Off\n");
//#endif
//					}
//					else
//					{
//						spcsys.cas.motor = 1;
//#ifdef DEBUG_MODE
//						printf("Motor On\n");
//#endif
//						spcsys.cas.startTime = (spcsys.tick * 125)+((4000-spcsys.Z80R.ICount) >> 5);
//						ResetCassette(&spcsys.cas);
//					}
//				}
//			}
//		}
//
//		if (spcsys.cas.button == CAS_REC && spcsys.cas.motor)
//		{
//			CasWrite(&spcsys.cas, Value & 0x01);
//		}
//	}
	else if ((Port & 0xFFFE) == 0x4000) // PSG
	{

		if (Port & 0x01) // Data
		{
		    if (spcsys.psgRegNum == 15) // Line Printer
           {
               if (Value != 0)
               {
                   //spcsys.prt.bufs[spcsys.prt.length++] = Value;
//                    printf("PRT <- %c (%d)\n", Value, Value);
//                    printf("%s(%d)\n", spcsys.prt.bufs, spcsys.prt.length);
               }
           }
			//Write8910(&spcsys.ay8910, (byte) spcsys.psgRegNum, Value);
		}
		else // Reg Num
		{
			spcsys.psgRegNum = Value;
			//WrCtrl8910(&spcsys.ay8910, Value);
		}
	}	
}

byte InZ80(register word Port)
{
	if (Port >= 0x8000 && Port <= 0x8009) // Keyboard Matrix
	{
		return keyMatrix[Port&0xf];
	}
	else if ((Port & 0xE000) == 0xA000) // IPLK
	{
		spcsys.IPLK = !spcsys.IPLK;
	} else if ((Port & 0xE000) == 0x2000) // GMODE
	{
		return spcsys.GMODE;
	} else if ((Port & 0xE000) == 0x0000) // VRAM reading
	{
		return spcsys.VRAM[Port];
	}	else if ((Port & 0xFFFE) == 0x4000) // PSG
	{
		byte retval = 0x1f;
		if (Port & 0x01) // Data
		{
			if (spcsys.psgRegNum == 14)
			{
				// 0x80 - cassette data input
				// 0x40 - motor status
				// 0x20 - print status
//				if (spcsys.prt.poweron)
//                {
//                    printf("Print Ready Check.\n");
//                    retval &= 0xcf;
//                }
//                else
//                {
//                    retval |= 0x20;
//                }
				if (spcsys.cas.button == CAS_PLAY && spcsys.cas.motor)
				{
					if (CasRead(&spcsys.cas) == 1)
							retval |= 0x80; // high
						else
							retval &= 0x7f; // low
				}
				if (spcsys.cas.motor)
					retval &= (~(0x40)); // 0 indicates Motor On
				else
					retval |= 0x40;

			}
			else 
			{
				int data = 0;// = RdData8910(&spcsys.ay8910);
				//printf("r(%d,%d)\n", spcsys.psgRegNum, data);
				return data;
			}
		} else if (Port & 0x02)
		{
            retval = (ReadVal() == 1 ? retval | 0x80 : retval & 0x7f);
		}
		return retval;
	}
	return 0;
}

#define STONE 56
#define LTONE (STONE*2)
int CasRead(CassetteTape *cas)
{
	int curTime;
	int bitTime;
	int ret = 0;
	int t;

	t = (spcsys.cycles - cas->lastTime) >> 5;
	if (t > (cas->rdVal ? LTONE : STONE))
	{
		cas->rdVal = ReadVal();
		//printf("%d",cas->rdVal);
		cas->lastTime = spcsys.cycles;
		t = (spcsys.cycles - cas->lastTime) >> 5;		
	}
	switch (cas->rdVal)
	{
	case 0:
		if (t > STONE/2)
			ret = 1; // high
		else
			ret = 0; // low
        break;
	case 1:
		if (t > STONE)
			ret = 1; // high
		else
			ret = 0; // low
	}
	return ret; // low for other cases
}

void CasWrite(CassetteTape *cas, int val)
{
	Uint32 curTime;
	int t;

	t = (spcsys.cycles - cas->lastTime) >> 5;
	if (t > 100)
		cas->cnt0 = cas->cnt1 = 0;
	cas->lastTime = spcsys.cycles;
	if (cas->wrVal == 1)
	{
		if (val == 0)
			if (t > STONE/2) 
			{
//				printf("1");
//				cas->cnt0 = 0;
//				fputc('1', spconf.wfp);
			} else {
				if (cas->cnt0++ < 100)
				{
//					printf("0");
//					fputc('0', spconf.wfp);
				}
			}
	}
	cas->wrVal = val;
}

word LoopZ80(register Z80 *R) {
	return 0;
}

void PatchZ80(register Z80 *R) {
	return;
}

int printf(const char *format, ...)
{
	va_list args;
	va_start(args,format);
	CString str;
	str.FormatV(format, args);
	va_end(args);
	//m_Logger.Write (str);
	return 1;
}
void memset(byte *p, int length, int b)
{
	for (int i=0; i<length; i++)
		*p++ = b;
}