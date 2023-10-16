/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundModem

QtSoundModem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundModem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundModem.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

#include "UZ7HOStuff.h"

/*
uses sysutils,classes;

  procedure KISS_init;
  procedure KISS_free;
  procedure KISS_add_stream(socket: integer);
  procedure KISS_del_stream(socket: integer);
  procedure KISS_on_data_in(socket: integer; data: string);
  procedure KISS_on_data_out(port: byte; frame: string);
  procedure KISS_send_ack(port: byte; data: string);
  procedure KISS_send_ack1(port: byte);
*/
// I don't like this. maybe fine for Dephi but overcomlicated for C

// I think I need a struct for each connection, but a simple array of entries should be fine
// My normal ** and count system
// Each needs an input buffer of max size kiss frame and length (or maybe string is a good idea)

TKISSMode ** KissConnections = NULL;
int KISSConCount = 0;

#define FEND 0xc0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD
#define KISS_ACKMODE 0x0C
#define KISS_DATA 0

struct TKISSMode_t  KISS;

int KISS_encode(UCHAR * KISSBuffer, int port, string * frame, int TXMON);

void KISS_init()
{
	int i;

	KISS.data_in = newString();

//	initTStringList(KISS.socket);

	for (i = 0; i < 4; i++)
	{
		initTStringList(&KISS.buffer[i]);
	}
}


/*
procedure KISS_free;
var
  i: byte;
begin
  KISS.data_in.Free;
  KISS.socket.Free;
  for i:=1 to 4 do
  begin
    KISS.buffer[i].Free;
    KISS.request[i].Free;
    KISS.acked[i].Free;
    KISS.irequest[i].Free;
    KISS.iacked[i].Free;
  end;
end;
*/

void KISS_add_stream(void * Socket)
{
	// Add a new connection. Called when QT accepts an incoming call}

	TKISSMode * KISS;

	KissConnections = realloc(KissConnections, (KISSConCount + 1) * sizeof(void *));

	KISS = KissConnections[KISSConCount++] = malloc(sizeof(KISS));

	KISS->Socket = Socket;
	KISS->data_in = newString();

}

void KISS_del_socket(void * socket)
{
	int i;
	
	TKISSMode * KISS = NULL;

	if (KISSConCount == 0)
		return;

	for (i = 0; i < KISSConCount; i++)
	{
		if (KissConnections[i]->Socket == socket)
		{
			KISS = KissConnections[i];
			break;
		}
	}

	if (KISS == NULL)
		return;

	// Need to remove entry and move others down

	KISSConCount--;

	while (i < KISSConCount)
	{
		KissConnections[i] = KissConnections[i + 1];
		i++;
	}
}


void KISS_on_data_out(int port, string * frame, int TX)
{
	int Len;
	UCHAR * KISSFrame = (UCHAR *)malloc(512);			// cant pass local data via signal/slot

	Len = KISS_encode(KISSFrame, port, frame, TX);

	KISSSendtoServer(NULL, KISSFrame, Len);				// Send to all open sockets
}

void ProcessKISSFrame(void * socket, UCHAR * Msg, int Len)
{
	int n = Len;
	UCHAR c;
	int ESCFLAG = 0;
	UCHAR * ptr1, *ptr2;
	int Chan;
	int Opcode;
	string * TXMSG;
	unsigned short CRC;
	UCHAR CRCString[2];

	ptr1 = ptr2 = Msg;

	while (n--)
	{
		c = *(ptr1++);

		if (ESCFLAG)
		{
			//
			//	FESC received - next should be TFESC or TFEND

			ESCFLAG = 0;

			if (c == TFESC)
				c = FESC;

			if (c == TFEND)
				c = FEND;

		}
		else
		{
			switch (c)
			{
			case FEND:

				//
				//	Either start of message or message complete
				//

				//	npKISSINFO->MSGREADY = TRUE;
				return;

			case FESC:

				ESCFLAG = 1;
				continue;

			}
		}
			
		//
		//	Ok, a normal char
		//

		*(ptr2++) = c;
		
	}
	Len = ptr2 - Msg;

	Chan = (Msg[0] >> 4);
	Opcode = Msg[0] & 0x0f;

	if (Chan > 3)
		return;

	switch (Opcode)
	{
	case KISS_ACKMODE:

		// How best to do ACKMODE?? I think pass whole frame including CMD and ack bytes to all_frame_buf

		// But ack should only be sent to client that sent the message - needs more thought!

		TXMSG = newString();
		stringAdd(TXMSG, &Msg[0], Len);		// include  Control

		CRC = get_fcs(&Msg[3], Len - 3);	// exclude control and ack bytes

		CRCString[0] = CRC & 0xff;
		CRCString[1] = CRC >> 8;

		stringAdd(TXMSG, CRCString, 2);

		// Ackmode needs to know where to send ack back to, so save socket on end of data

		stringAdd(TXMSG, (unsigned char * )&socket, sizeof(socket));

		// if KISS Optimise see if frame is really needed

		if (!KISS_opt[Chan])
			Add(&KISS.buffer[Chan], TXMSG);
		else
		{
			if (add_raw_frames(Chan, TXMSG, &KISS.buffer[Chan]))
				Add(&KISS.buffer[Chan], TXMSG);
		}

		return;

	case KISS_DATA:

		TXMSG = newString();
		stringAdd(TXMSG, &Msg[0], Len);		// include Control

		CRC = get_fcs(&Msg[1], Len - 1);

		CRCString[0] = CRC & 0xff;
		CRCString[1] = CRC >> 8;

		stringAdd(TXMSG, CRCString, 2);

		// if KISS Optimise see if frame is really needed

		if (!KISS_opt[Chan])
			Add(&KISS.buffer[Chan], TXMSG);
		else
		{
			if (add_raw_frames(Chan, TXMSG, &KISS.buffer[Chan]))
				Add(&KISS.buffer[Chan], TXMSG);
		}


		return;
	}

	// Still need to process kiss control frames
}




void KISSDataReceived(void * socket, UCHAR * data, int length)
{
	int i;
	UCHAR * ptr1, * ptr2;
	int Length;

	TKISSMode * KISS = NULL;

	if (KISSConCount == 0)
		return;

	for (i = 0; i < KISSConCount; i++)
	{
		if (KissConnections[i]->Socket == socket)
		{
			KISS = KissConnections[i];
			break;
		}
	}

	if (KISS == NULL)
		return;

	stringAdd(KISS->data_in, data, length);

	if (KISS->data_in->Length > 10000)				// Probably AGW Data on KISS Port
	{
		KISS->data_in->Length = 0;
		return;
	}

	ptr1 = KISS->data_in->Data;
	Length = KISS->data_in->Length;


	while ((ptr2 = memchr(ptr1, FEND, Length)))
	{
		int Len = (ptr2 - ptr1);

		if (Len == 0)
		{
			// Start of frame

			mydelete(KISS->data_in, 0, 1);

			ptr1 = KISS->data_in->Data;
			Length = KISS->data_in->Length;

			continue;
		}

		// Process Frame

		if (Len < 350)								// Drop obviously corrupt frames
			ProcessKISSFrame(socket, ptr1, Len);		

		mydelete(KISS->data_in, 0, Len + 1);

		ptr1 = KISS->data_in->Data;
		Length = KISS->data_in->Length;

	}

	/*			if (length(KISS.data_in.Strings[idx]) > 65535)
					if Form1.ServerSocket2.Socket.ActiveConnections > 0)

				  for i:=0 to Form1.ServerSocket2.Socket.ActiveConnections-1 do
					if Form1.ServerSocket2.Socket.Connections[i].SocketHandle=socket then
					   try Form1.ServerSocket2.Socket.Connections[i].Close; except end;
		*/


}



int KISS_encode(UCHAR * KISSBuffer, int port, string * frame, int TXMON)
{

	//	Encode frame

	UCHAR * ptr1 = frame->Data;
	UCHAR TXCCC = 0;
	int Len = frame->Length - 2;		// frame includes CRC
	UCHAR * ptr2 = &KISSBuffer[2];
	UCHAR c;

	if (TXMON)
	{
		// TX Frame has control byte on front

		ptr1++;
		Len--;
	}

	KISSBuffer[0] = FEND;
	KISSBuffer[1] = port << 4;

	TXCCC ^= KISSBuffer[1];

	while (Len--)
	{
		c = *(ptr1++);
		TXCCC ^= c;

		switch (c)
		{
		case FEND:
			(*ptr2++) = FESC;
			(*ptr2++) = TFEND;
			break;

		case FESC:

			(*ptr2++) = FESC;
			(*ptr2++) = TFESC;
			break;

			// Drop through

		default:

			(*ptr2++) = c;
		}
	}

	// If using checksum, send it
/*

	if (KISSFLAGS & CHECKSUM)
	{
		c = (UCHAR)KISS->TXCCC;

		// On TNC-X based boards, it is difficult to cope with an encoded CRC, so if
		// CRC is FEND, send it as 0xc1. This means we have to accept 00 or 01 as valid.
		// which is a slight loss in robustness

		if (c == FEND && (PORT->KISSFLAGS & TNCX))
		{
			(*ptr2++) = FEND + 1;
		}
		else
		{
			switch (c)
			{
			case FEND:
				(*ptr2++) = FESC;
				(*ptr2++) = TFEND;
				break;

			case FESC:
				(*ptr2++) = FESC;
				(*ptr2++) = TFESC;
				break;

			default:
				(*ptr2++) = c;
			}
		}
	}
	*/

	(*ptr2++) = FEND;

	return (int)(ptr2 - KISSBuffer);
}


void sendAckModeAcks(int snd_ch)
{
	// format and send any outstanding acks

	string * temp;
	UCHAR * Msg;
	void * socket;

	while (KISS_acked[snd_ch].Count)
	{
		UCHAR * ACK = (UCHAR *)malloc(15);
		UCHAR * ackptr = ACK;

		temp = Strings(&KISS_acked[snd_ch], 0);	// get first
		Msg = temp->Data;

		*ackptr++ = FEND;
		*ackptr++ = Msg[0];			// opcode and channel

		*ackptr++ = Msg[1];
		*ackptr++ = Msg[2];			// ACK Bytes
		*ackptr++ = FEND;

		// Socket to reply to is on end

		Msg += (temp->Length - sizeof(void *));

		memcpy(&socket, Msg, sizeof(void *));

		KISSSendtoServer(socket, ACK, 5);
		Delete(&KISS_acked[snd_ch], 0);			// This will invalidate temp
	}
}







