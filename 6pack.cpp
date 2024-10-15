/*

Using code from 6pack Linux Kernel driver with the following licence and credits

 *	6pack driver version 0.4.2, 1999/08/22
 *
 *	This module:
 *		This module is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * 		This module implements the AX.25 protocol for kernel-based
 *		devices like TTYs. It interfaces between a raw TTY, and the
 *		kernel's AX.25 protocol layers, just like slip.c.
 *		AX.25 needs to be separated from slip.c while slip.c is no
 *		longer a static kernel device since it is a module.
 *
 *	Author: Andreas Könsgen <ajk@ccac.rwth-aachen.de>
 *
 *	Lots of stuff has been taken from mkiss.c, written by
 *	Hans Alblas <hans@esrac.ele.tue.nl>
 *
 *	with the fixes from
 *
 *	Jonathan (G4KLX)	Fixed to match Linux networking changes - 2.1.15.
 *	Matthias (DG2FEF)       Fixed bug in ax25_close(): dev_lock_wait() was
 *                              called twice, causing a deadlock.
 */


 //	6pack needs fast response to received characters, and I want to be able to operate over TCP links as well as serial.
 //	So I think the character level stuff may need to run in a separate thread, probably using select.
 //
 //	I also need to support multiple 6pack ports.

 // ?? Do we add this as a backend to KISS driver or a separate Driver. KISS Driver is already quite messy. Not decided yet.

 // ?? If using serial/real TNC we need to be able to interleave control and data bytes, but I think with TCP/QtSM it won't be necessary
 // ?? Also a don't see any point in running multiple copies of QtSM on one port, but maybe should treat the QtSM channels as
 //	multidropped ports for scheduling (?? only if on same radio ??)

 //	?? I think it needs to look like a KISS (L2) driver but will need a transmit scheduler level to do DCD/CSMA/PTT processing,
 //	ideally with an interlock to other drivers on same port. This needs some thought with QtSM KISS with multiple modems on one channel





#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

/****************************************************************************
 *	Defines for the 6pack driver.
 ****************************************************************************/

#define TRUE	1
#define FALSE	0

#define AX25_MAXDEV	16		/* MAX number of AX25 channels;
					   This can be overridden with
					   insmod -oax25_maxdev=nnn	*/
#define AX_MTU		236	

					   /* 6pack protocol bytes/masks. */
#define SIXP_INIT_CMD		0xE8
#define SIXP_TNC_FOUND		0xE9
#define SIXP_CMD_MASK		0xC0
#define SIXP_PRIO_CMD_MASK	0x80
#define SIXP_PRIO_DATA_MASK	0x38
#define SIXP_STD_CMD_MASK	0x40
#define SIXP_DCD_MASK		0x08
#define SIXP_RX_DCD_MASK	0x18
#define SIXP_CHN_MASK		0x07
#define SIXP_TX_MASK		0x20
#define SIXP_CON_LED_ON		0x68
#define SIXP_STA_LED_ON		0x70
#define SIXP_LED_OFF		0x60

/* checksum for a valid 6pack encapsulated packet */
#define SIXP_CHKSUM		0xFF

/* priority commands */
#define SIXP_SEOF		0x40	/* TX underrun */
#define SIXP_TX_URUN		0x48	/* TX underrun */
#define SIXP_RX_ORUN		0x50	/* RX overrun */
#define SIXP_RX_BUF_OVL		0x58	/* RX overrun */

struct ax_disp {
	int                magic;

	char * name;
	/* Various fields. */
//	struct tty_struct  *tty;		/* ptr to TTY structure		*/
//	struct device      *dev;		/* easy for intr handling	*/
	struct ax_disp     *sixpack;		/* mkiss txport if mkiss channel*/

	/* These are pointers to the malloc()ed frame buffers. */
	unsigned char      *rbuff;		/* receiver buffer		*/
	int                rcount;		/* received chars counter       */
	unsigned char      *xbuff;		/* transmitter buffer		*/
	unsigned char      *xhead;		/* pointer to next byte to XMIT */
	int                xleft;		/* bytes left in XMIT queue     */

	/* SLIP interface statistics. */
	unsigned long      rx_packets;		/* inbound frames counter	*/
	unsigned long      tx_packets;		/* outbound frames counter      */
	unsigned long      rx_errors;		/* Parity, etc. errors          */
	unsigned long      tx_errors;		/* Planned stuff                */
	unsigned long      rx_dropped;		/* No memory for skb            */
	unsigned long      tx_dropped;		/* When MTU change              */
	unsigned long      rx_over_errors;	/* Frame bigger then SLIP buf.  */

	/* Detailed SLIP statistics. */
	int                 mtu;		/* Our mtu (to spot changes!)   */
	int                 buffsize;		/* Max buffers sizes            */


	unsigned char       flags;		/* Flag values/ mode etc	*/
#define AXF_INUSE	0		/* Channel in use               */
#define AXF_ESCAPE	1               /* ESC received                 */
#define AXF_ERROR	2               /* Parity, etc. error           */
#define AXF_KEEPTEST	3		/* Keepalive test flag		*/
#define AXF_OUTWAIT	4		/* is outpacket was flag	*/

	int                 mode;


	/* variables for the state machine */
	unsigned char	tnc_ok;
	unsigned char	status;
	unsigned char	status1;
	unsigned char	status2;

	unsigned char	duplex;
	unsigned char	led_state;
	unsigned char	tx_enable;

	unsigned char	raw_buf[4];		/* receive buffer */
	unsigned char	cooked_buf[400];	/* receive buffer after 6pack decoding */

	unsigned int	rx_count;		/* counter for receive buffer */
	unsigned int	rx_count_cooked;	/* counter for receive buffer after 6pack decoding */

	unsigned char	tx_delay;
	unsigned char	persistance;
	unsigned char	slottime;

};

struct sixpack_channel {
	int magic;		/* magic word */
	int init;		/* channel exists? */
	struct tty_struct *tty; /* link to tty control structure */
};

#define AX25_MAGIC		0x5316
#define SIXP_DRIVER_MAGIC	0x5304

#define SIXP_INIT_RESYNC_TIMEOUT	150	/* in 10 ms */
#define SIXP_RESYNC_TIMEOUT		500	/* in 10 ms */

/* default radio channel access parameters */
#define SIXP_TXDELAY			25	/* in 10 ms */
#define SIXP_PERSIST			50
#define SIXP_SLOTTIME			10	/* in 10 ms */

static int sixpack_encaps(unsigned char *tx_buf, unsigned char *tx_buf_raw, int length, unsigned char tx_delay);
static void sixpack_decaps(struct ax_disp *, unsigned char);

static void decode_prio_command(unsigned char, struct ax_disp *);
static void decode_std_command(unsigned char, struct ax_disp *);
static void decode_data(unsigned char, struct ax_disp *);
static void resync_tnc(unsigned long);
static void xmit_on_air(struct ax_disp *ax);
static void start_tx_timer(struct ax_disp *ax);

extern "C" void Debugprintf(const char * format, ...);

void Process6PackByte(unsigned char inbyte);

struct ax_disp axdisp;

/* Send one completely decapsulated AX.25 packet to the AX.25 layer. */
static void ax_bump(struct ax_disp *ax)
{
}


void Process6PackData(unsigned char * Bytes, int Len)
{
	while(Len--)
		Process6PackByte(Bytes++[0]);

}

void Process6PackByte(unsigned char inbyte)
{
	struct ax_disp *ax = &axdisp;

	if (inbyte == SIXP_INIT_CMD)
	{
		Debugprintf("6pack: SIXP_INIT_CMD received.\n");
		{
			// Reset state machine and allocate a 6pack struct for each modem.

			// reply with INIT_CMD with the channel no of last modem
		}
		return;
	}

	if ((inbyte & SIXP_PRIO_CMD_MASK) != 0)
		decode_prio_command(inbyte, ax);
	else if ((inbyte & SIXP_STD_CMD_MASK) != 0)
		decode_std_command(inbyte, ax);
	else {
		if ((ax->status & SIXP_RX_DCD_MASK) == SIXP_RX_DCD_MASK)
			decode_data(inbyte, ax);
	} /* else */
}

/* identify and execute a 6pack priority command byte */

void decode_prio_command(unsigned char cmd, struct ax_disp *ax)
{
	unsigned char channel;

	channel = cmd & SIXP_CHN_MASK;
	if ((cmd & SIXP_PRIO_DATA_MASK) != 0) {     /* idle ? */

	/* RX and DCD flags can only be set in the same prio command,
	   if the DCD flag has been set without the RX flag in the previous
	   prio command. If DCD has not been set before, something in the
	   transmission has gone wrong. In this case, RX and DCD are
	   cleared in order to prevent the decode_data routine from
	   reading further data that might be corrupt. */

		if (((ax->status & SIXP_DCD_MASK) == 0) &&
			((cmd & SIXP_RX_DCD_MASK) == SIXP_RX_DCD_MASK)) {
			if (ax->status != 1)
				Debugprintf("6pack: protocol violation\n");
			else
				ax->status = 0;
			cmd &= !SIXP_RX_DCD_MASK;
		}
		ax->status = cmd & SIXP_PRIO_DATA_MASK;
	} /* if */


		/* if the state byte has been received, the TNC is present,
		   so the resync timer can be reset. */

	if (ax->tnc_ok == 1) {
		//		del_timer(&(ax->resync_t));
		//		ax->resync_t.data = (unsigned long) ax;
		//		ax->resync_t.function = resync_tnc;
		//		ax->resync_t.expires = jiffies + SIXP_INIT_RESYNC_TIMEOUT;
		//		add_timer(&(ax->resync_t));
	}

	ax->status1 = cmd & SIXP_PRIO_DATA_MASK;
}

/* try to resync the TNC. Called by the resync timer defined in
  decode_prio_command */

static void
resync_tnc(unsigned long channel)
{
	static char resync_cmd = SIXP_INIT_CMD;
	struct ax_disp *ax = (struct ax_disp *) channel;

	Debugprintf("6pack: resyncing TNC\n");

	/* clear any data that might have been received */

	ax->rx_count = 0;
	ax->rx_count_cooked = 0;

	/* reset state machine */

	ax->status = 1;
	ax->status1 = 1;
	ax->status2 = 0;
	ax->tnc_ok = 0;

	/* resync the TNC */

	ax->led_state = SIXP_LED_OFF;
	//	ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
	//	ax->tty->driver.write(ax->tty, 0, &resync_cmd, 1);


		/* Start resync timer again -- the TNC might be still absent */

	//	del_timer(&(ax->resync_t));
	//	ax->resync_t.data = (unsigned long) ax;
	//	ax->resync_t.function = resync_tnc;
	//	ax->resync_t.expires = jiffies + SIXP_RESYNC_TIMEOUT;
	//	add_timer(&(ax->resync_t));
}



/* identify and execute a standard 6pack command byte */

void decode_std_command(unsigned char cmd, struct ax_disp *ax)
{
	unsigned char checksum = 0, channel;
	unsigned int i;

	channel = cmd & SIXP_CHN_MASK;
	switch (cmd & SIXP_CMD_MASK) {     /* normal command */
	case SIXP_SEOF:
		if ((ax->rx_count == 0) && (ax->rx_count_cooked == 0)) {
			if ((ax->status & SIXP_RX_DCD_MASK) ==
				SIXP_RX_DCD_MASK) {
				ax->led_state = SIXP_CON_LED_ON;
				//					ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
			} /* if */
		}
		else {
			ax->led_state = SIXP_LED_OFF;
			//				ax->tty->driver.write(ax->tty, 0, &(ax->led_state), 1);
							/* fill trailing bytes with zeroes */
			if (ax->rx_count == 2) {
				decode_data(0, ax);
				decode_data(0, ax);
				ax->rx_count_cooked -= 2;
			}
			else if (ax->rx_count == 3) {
				decode_data(0, ax);
				ax->rx_count_cooked -= 1;
			}
			for (i = 0; i < ax->rx_count_cooked; i++)
				checksum += ax->cooked_buf[i];
			if (checksum != SIXP_CHKSUM) {
				Debugprintf("6pack: bad checksum %2.2x\n", checksum);
			}
			else {
				ax->rcount = ax->rx_count_cooked - 1;
				ax_bump(ax);
			} /* else */
			ax->rx_count_cooked = 0;
		} /* else */
		break;
	case SIXP_TX_URUN:
		Debugprintf("6pack: TX underrun\n");
		break;
	case SIXP_RX_ORUN:
		Debugprintf("6pack: RX overrun\n");
		break;
	case SIXP_RX_BUF_OVL:
		Debugprintf("6pack: RX buffer overflow\n");
	} /* switch */
} /* function */

/* decode 4 sixpack-encoded bytes into 3 data bytes */

void decode_data(unsigned char inbyte, struct ax_disp *ax)
{
	unsigned char *buf;

	if (ax->rx_count != 3)
		ax->raw_buf[ax->rx_count++] = inbyte;
	else {
		buf = ax->raw_buf;
		ax->cooked_buf[ax->rx_count_cooked++] =
			buf[0] | ((buf[1] << 2) & 0xc0);
		ax->cooked_buf[ax->rx_count_cooked++] =
			(buf[1] & 0x0f) | ((buf[2] << 2) & 0xf0);
		ax->cooked_buf[ax->rx_count_cooked++] =
			(buf[2] & 0x03) | (inbyte << 2);
		ax->rx_count = 0;
	}
}
