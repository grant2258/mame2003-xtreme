/***************************************************************************

  Edward Randy      (c) 1990 Data East Corporation (World version)
  Edward Randy      (c) 1990 Data East Corporation (Japanese version)
  Caveman Ninja     (c) 1991 Data East Corporation (World version)
  Caveman Ninja     (c) 1991 Data East Corporation (USA version)
  Joe & Mac         (c) 1991 Data East Corporation (Japanese version)
  Robocop 2         (c) 1991 Data East Corporation (USA version)
  Robocop 2         (c) 1991 Data East Corporation (Japanese version)
  Robocop 2         (c) 1991 Data East Corporation (World version)
  Stone Age         (Italian bootleg)
  Mutant Fighter	(c) 1992 Data East Corporation (World version)
  Death Brade		(c) 1992 Data East Corporation (Japanese version)

  Edward Randy runs on the same board as Caveman Ninja but the protection
  chip is different.  Robocop 2 also has a different protection chip but
  strangely makes very little use of it (only one check at the start).
  Robocop 2 is a different board but similar hardware.

  Edward Randy (World rev 1) seems much more polished than World rev 2 -
  better attract mode at least.

  The sound program of Stoneage is ripped from Block Out (by Technos!)

  Mutant Fighter introduced alpha-blending to this basic board design.
  The characters shadows sometimes jump around a little - a bug in the
  original board, not the emulation.

Caveman Ninja Issues:
  End of level 2 is corrupt.

  Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "cninja.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "deco16ic.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "ost_samples.h"

static int cninja_scanline, cninja_irq_mask;
static void *raster_irq_timer;
static data16_t *cninja_ram;

/**********************************************************************************/

static WRITE16_HANDLER( cninja_sound_w )
{

	if( ost_support_enabled(OST_SUPPORT_CNINJA) )
	{

		if(generate_ost_sound( data ))
		{
			soundlatch_w(0,data&0xff);
			cpu_set_irq_line(1,0,HOLD_LINE);
		}
	}
	else
	{
		soundlatch_w(0,data&0xff);
		cpu_set_irq_line(1,0,HOLD_LINE);
	}
}

static WRITE16_HANDLER( stoneage_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static void interrupt_gen(int scanline)
{
	/* Save state of scroll registers before the IRQ */
	deco16_raster_display_list[deco16_raster_display_position++]=scanline;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf12_control[1]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf12_control[2]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf12_control[3]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf12_control[4]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf34_control[1]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf34_control[2]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf34_control[3]&0xffff;
	deco16_raster_display_list[deco16_raster_display_position++]=deco16_pf34_control[4]&0xffff;

	cpu_set_irq_line(0, (cninja_irq_mask&0x10) ? 3 : 4, ASSERT_LINE);
	timer_adjust(raster_irq_timer,TIME_NEVER,0,0);
}

static READ16_HANDLER( cninja_irq_r )
{
	switch (offset) {

	case 1: /* Raster IRQ scanline position */
		return cninja_scanline;

	case 2: /* Raster IRQ ACK - value read is not used */
		cpu_set_irq_line(0, 3, CLEAR_LINE);
		cpu_set_irq_line(0, 4, CLEAR_LINE);
		return 0;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Unmapped IRQ read %d\n",activecpu_get_pc(),offset);
	return 0;
}

static WRITE16_HANDLER( cninja_irq_w )
{
	switch (offset) {
	case 0:
		/* IRQ enable:
			0xca:	Raster IRQ turned off
			0xc8:	Raster IRQ turned on (68k IRQ level 4)
			0xd8:	Raster IRQ turned on (68k IRQ level 3)
		*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  IRQ write %d %08x\n",activecpu_get_pc(),offset,data);
		cninja_irq_mask=data&0xff;
		return;

	case 1: /* Raster IRQ scanline position, only valid for values between 1 & 239 (0 and 240-256 do NOT generate IRQ's) */
		cninja_scanline=data&0xff;
		if ((cninja_irq_mask&0x2)==0 && cninja_scanline>0 && cninja_scanline<240)
			timer_adjust(raster_irq_timer,cpu_getscanlinetime(cninja_scanline),cninja_scanline,TIME_NEVER);
		else
			timer_adjust(raster_irq_timer,TIME_NEVER,0,0);
		return;

	case 2: /* VBL irq ack */
		return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Unmapped IRQ write %d %04x\n",activecpu_get_pc(),offset,data);
}

static READ16_HANDLER( robocop2_prot_r )
{
 	switch (offset<<1) {
		case 0x41a: /* Player 1 & 2 input ports */
			return readinputport(0);
		case 0x320: /* Coins */
			return readinputport(1);
		case 0x4e6: /* Dip switches */
			return readinputport(2);
		case 0x504: /* PC: 6b6.  b4, 2c, 36 written before read */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset);
			return 0x84;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Protection PC %06x: warning - read unmapped memory address %04x\n",activecpu_get_pc(),offset);
	return 0;
}

/**********************************************************************************/

static MEMORY_READ16_START( cninja_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x146000, 0x146fff, MRA16_RAM },
	{ 0x14e000, 0x14e7ff, MRA16_RAM },
	{ 0x154000, 0x154fff, MRA16_RAM },
	{ 0x156000, 0x156fff, MRA16_RAM },
	{ 0x15c000, 0x15c7ff, MRA16_RAM },
	{ 0x15e000, 0x15e7ff, MRA16_RAM },
	{ 0x184000, 0x187fff, MRA16_RAM },
	{ 0x190000, 0x190007, cninja_irq_r },
	{ 0x19c000, 0x19dfff, MRA16_RAM },
	{ 0x1a4000, 0x1a47ff, MRA16_RAM }, /* Sprites */
	{ 0x1bc000, 0x1bcfff, deco16_104_cninja_prot_r }, /* Protection device */
MEMORY_END

static MEMORY_WRITE16_START( cninja_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },

	{ 0x140000, 0x14000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x144000, 0x144fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x146000, 0x146fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &deco16_pf2_rowscroll },

	{ 0x150000, 0x15000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x154000, 0x154fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x156000, 0x156fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &deco16_pf4_rowscroll },

	{ 0x184000, 0x187fff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x190000, 0x190007, cninja_irq_w },
	{ 0x19c000, 0x19dfff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x1a4000, 0x1a47ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x1b4000, 0x1b4001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1bc000, 0x1bc0ff, deco16_104_cninja_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x308000, 0x308fff, MWA16_NOP }, /* Bootleg only */
MEMORY_END

static MEMORY_READ16_START( edrandy_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x146000, 0x146fff, MRA16_RAM },
	{ 0x14c000, 0x14c7ff, MRA16_RAM },
	{ 0x14e000, 0x14e7ff, MRA16_RAM },
	{ 0x154000, 0x154fff, MRA16_RAM },
	{ 0x156000, 0x156fff, MRA16_RAM },
	{ 0x15c000, 0x15c7ff, MRA16_RAM },
	{ 0x15e000, 0x15e7ff, MRA16_RAM },
	{ 0x188000, 0x189fff, MRA16_RAM },
	{ 0x194000, 0x197fff, MRA16_RAM },
	{ 0x198000, 0x1987ff, deco16_60_prot_r }, /* Protection device */
	{ 0x1a4000, 0x1a4007, cninja_irq_r },
	{ 0x1bc000, 0x1bc7ff, MRA16_RAM }, /* Sprites */
MEMORY_END

static MEMORY_WRITE16_START( edrandy_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },

	{ 0x140000, 0x14000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x144000, 0x144fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x146000, 0x146fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &deco16_pf2_rowscroll },

	{ 0x150000, 0x15000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x154000, 0x154fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x156000, 0x156fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &deco16_pf4_rowscroll },

	{ 0x188000, 0x189fff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x194000, 0x197fff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x198000, 0x1987ff, deco16_60_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x199550, 0x199551, MWA16_NOP }, /* Looks like a bug in game code, a protection write is referenced off a5 instead of a6 and ends up here */
	{ 0x199750, 0x199751, MWA16_NOP }, /* Looks like a bug in game code, a protection write is referenced off a5 instead of a6 and ends up here */

	{ 0x1a4000, 0x1a4007, cninja_irq_w },
	{ 0x1ac000, 0x1ac001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1bc000, 0x1bc7ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x1bc800, 0x1bcfff, MWA16_NOP }, /* Another bug in game code?  Sprite list can overrun.  Doesn't seem to mirror */
MEMORY_END

static MEMORY_READ16_START( robocop2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x180000, 0x1807ff, MRA16_RAM },
	{ 0x18c000, 0x18c7ff, robocop2_prot_r }, /* Protection device */
	{ 0x1a8000, 0x1a9fff, MRA16_RAM },
	{ 0x1b0000, 0x1b0007, cninja_irq_r },
	{ 0x1b8000, 0x1bbfff, MRA16_RAM },
	{ 0x144000, 0x144fff, MRA16_RAM },
	{ 0x146000, 0x146fff, MRA16_RAM },
	{ 0x14c000, 0x14c7ff, MRA16_RAM },
	{ 0x14e000, 0x14e7ff, MRA16_RAM },
	{ 0x154000, 0x154fff, MRA16_RAM },
	{ 0x156000, 0x156fff, MRA16_RAM },
	{ 0x15c000, 0x15c7ff, MRA16_RAM },
	{ 0x15e000, 0x15e7ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( robocop2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x140000, 0x14000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x144000, 0x144fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x146000, 0x146fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x14c000, 0x14c7ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x14e000, 0x14e7ff, MWA16_RAM, &deco16_pf2_rowscroll },

	{ 0x150000, 0x15000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x154000, 0x154fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x156000, 0x156fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x15c000, 0x15c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x15e000, 0x15e7ff, MWA16_RAM, &deco16_pf4_rowscroll },

	{ 0x180000, 0x1807ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x18c064, 0x18c065, cninja_sound_w },
/*	{ 0x18c000, 0x18c0ff, cninja_loopback_w },  // Protection writes /*/
	{ 0x198000, 0x198001, buffer_spriteram16_w }, /* DMA flag */
	{ 0x1a8000, 0x1a9fff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x1b0000, 0x1b0007, cninja_irq_w },
	{ 0x1b8000, 0x1bbfff, MWA16_RAM, &cninja_ram }, /* Main ram */
	{ 0x1f0000, 0x1f0001, deco16_priority_w },
MEMORY_END

static MEMORY_READ16_START( mutantf_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x161fff, MRA16_RAM },
	{ 0x1a0000, 0x1a07ff, deco16_66_prot_r }, /* Protection device */
	{ 0x1c0000, 0x1c0001, deco16_71_r },

	{ 0x304000, 0x305fff, MRA16_RAM },
	{ 0x306000, 0x307fff, MRA16_RAM },
	{ 0x308000, 0x3087ff, MRA16_RAM },
	{ 0x30a000, 0x30a7ff, MRA16_RAM },
	{ 0x314000, 0x315fff, MRA16_RAM },
	{ 0x316000, 0x317fff, MRA16_RAM },
	{ 0x318000, 0x3187ff, MRA16_RAM },
	{ 0x31a000, 0x31a7ff, MRA16_RAM },
	{ 0xad00ac, 0xad00ff, MRA16_NOP }, /* Reads from here seem to be a game code bug */
MEMORY_END

static MEMORY_WRITE16_START( mutantf_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM }, /* Main ram */
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x140000, 0x1407ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x160000, 0x161fff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x180000, 0x180001, deco16_priority_w },
	{ 0x180002, 0x180003, MWA16_NOP }, /* VBL irq ack */
	{ 0x1a0000, 0x1a07ff, deco16_66_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x1c0000, 0x1c0001, buffer_spriteram16_w },
	{ 0x1e0000, 0x1e0001, buffer_spriteram16_2_w },

	{ 0x300000, 0x30000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x304000, 0x305fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x306000, 0x307fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x308000, 0x3087ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x30a000, 0x30a7ff, MWA16_RAM, &deco16_pf2_rowscroll },

	{ 0x310000, 0x31000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x314000, 0x315fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x316000, 0x317fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x318000, 0x3187ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x31a000, 0x31a7ff, MWA16_RAM, &deco16_pf4_rowscroll },
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, YM2203_status_port_0_r },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, YM2203_word_0_w },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem_mutantf )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_mutantf )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

static MEMORY_READ_START( stoneage_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
MEMORY_END

static MEMORY_WRITE_START( stoneage_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

/**********************************************************************************/

INPUT_PORTS_START( edrandy )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Energy" )
	PORT_DIPSETTING(      0x0100, "2.5" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x0300, "3.5" )
	PORT_DIPSETTING(      0x0200, "4.5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) ) /* Not confirmed */
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Continues" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cninja )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Restore Life Meter" )	/* Life Meter Restored when Big Boss is Defeated */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cninjau )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Credit(s) to Start" ) /* Also, if Coin A and B are on 1/1, 0x00 gives 2 to start, 1 to continue */
	PORT_DIPSETTING(      0x0080, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( robocop2 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mutantf )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Continue Coin" )
	PORT_DIPSETTING(      0x0080, "Normal" )
	PORT_DIPSETTING(      0x0000, "Two Coin Start" )
	PORT_DIPNAME( 0x0300, 0x0300, "Timer Decrement" )
	PORT_DIPSETTING(      0x0100, "Slow" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0200, "Fast" )
	PORT_DIPSETTING(      0x0000, "Very Fast" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, "Life Per Stage" )
	PORT_DIPSETTING(      0x0000, "Least" )
	PORT_DIPSETTING(      0x1000, "Little" )
	PORT_DIPSETTING(      0x2000, "Less" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPNAME( 0x4000, 0x4000, "Continues" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0,  },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout tilelayout_8bpp =
{
	16,16,
	4096,
	8,
	{ 0x100000*8+8, 0x100000*8, 0x40000*8+8, 0x40000*8, 0xc0000*8+8, 0xc0000*8, 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,    0, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,  512, 64 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout,768, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_robocop2[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,    0, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,  512, 64 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout,768, 32 },	/* Sprites 16x16 */
	{ REGION_GFX3, 0, &tilelayout_8bpp,  512, 1 },	/* Tiles 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_mutantf[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 64 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,          0, 64 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,          0, 64 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout,      256, 128 },	/* Sprites 16x16 */
	{ REGION_GFX5, 0, &spritelayout,     1024+768, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static MACHINE_INIT( cninja )
{
	raster_irq_timer = timer_alloc(interrupt_gen);
	cninja_scanline=0;
	cninja_irq_mask=0;
}

static struct YM2203interface ym2203_interface =
{
	1,
	32220000/8, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM2203_VOL(60,60) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static void sound_irq2(int state)
{
	cpu_set_irq_line(1,0,state);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	/* the second OKIM6295 ROM is bank switched */
	OKIM6295_set_bank_base(1, (data & 1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct YM2151interface ym2151_interface2 =
{
	1,
	3579545,	/* 3.579545 MHz (?) */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ sound_irq2 }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 75, 60 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( cninja )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(cninja_readmem,cninja_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Accurate */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(cninja)
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(cninja)
	MDRV_VIDEO_UPDATE(cninja)
	MDRV_VIDEO_EOF(cninja)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_INSTALL_OST_SUPPORT(OST_SUPPORT_CNINJA)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( stoneage )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(cninja_readmem,cninja_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(stoneage_s_readmem,stoneage_s_writemem)

	MDRV_MACHINE_INIT(cninja)
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(stoneage)
	MDRV_VIDEO_UPDATE(cninja)
	MDRV_VIDEO_EOF(cninja)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface2)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( edrandy )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(edrandy_readmem,edrandy_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Accurate */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(cninja)
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(edrandy)
	MDRV_VIDEO_UPDATE(edrandy)
	MDRV_VIDEO_EOF(cninja)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( robocop2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(robocop2_readmem,robocop2_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Accurate */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_MACHINE_INIT(cninja)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_robocop2)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(robocop2)
	MDRV_VIDEO_UPDATE(robocop2)
	MDRV_VIDEO_EOF(cninja)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mutantf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(mutantf_readmem,mutantf_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem_mutantf,sound_writemem_mutantf)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_mutantf)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(mutantf)
	MDRV_VIDEO_UPDATE(mutantf)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( cninja )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gn02rev3.bin", 0x00000, 0x20000, CRC(39aea12a) SHA1(5de4e26d2c03c559249720b6a204567673754774) )
	ROM_LOAD16_BYTE( "gn05rev2.bin", 0x00001, 0x20000, CRC(0f4360ef) SHA1(d60b3377e818a037d0f94383dd207865853f529d) )
	ROM_LOAD16_BYTE( "gn01rev2.bin", 0x40000, 0x20000, CRC(f740ef7e) SHA1(e70bf04e2407dc0c512617417581388365eb1d35) )
	ROM_LOAD16_BYTE( "gn04rev2.bin", 0x40001, 0x20000, CRC(c98fcb62) SHA1(b2ee52a9418190c62e0b34920e44111270d68286) )
	ROM_LOAD16_BYTE( "gn-00.rom",    0x80000, 0x20000, CRC(0b110b16) SHA1(a967c8aeae3f0cee1f354583cf26ee736636aaf8) )
	ROM_LOAD16_BYTE( "gn-03.rom",    0x80001, 0x20000, CRC(1e28e697) SHA1(2313e97f3a34892dfdc338944c0f00538fcae800) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  CRC(ca8bef96) SHA1(fcdbd598c85e339a3389a2ef58cf2d5b3a2779af) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gl-08.rom",  0x00001,  0x10000,  CRC(33a2b400) SHA1(fdb8de315f33705719c0ac03a61fb56ffbfdf597) )	/* chars */
	ROM_LOAD16_BYTE( "gl-09.rom",  0x00000,  0x10000,  CRC(5a2d4752) SHA1(617dd10a99b5b55ca64dcdd22a0f133b0d6b770d) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  CRC(de89c69a) SHA1(b41bdf859854b5541c7eae7cd541b910cea1f839) )	/* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  CRC(a8f05d33) SHA1(a1330bc9ca4648219403db087622badfc632b47d) )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  CRC(5b399eed) SHA1(490f8f9c0c557b0ba94c6019e3fe680641a0787e) )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  CRC(2220eb9f) SHA1(bdf0bd6e6ba375f0770b9d08a7efa32201cbb6ef) )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  CRC(56a53254) SHA1(10940cfdc6fbe9013865107de3394ca7f782d9c7) )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  CRC(144b94cc) SHA1(d982508608942a714b428a2b721bf24e1627cbb6) )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  CRC(82d44749) SHA1(c471fa573e00c2f8ae44068439ba6d849a124c68) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  CRC(d92e519d) SHA1(08238f12bf7058a3965ab6348b468e3d35d4cd23) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  CRC(08eb5264) SHA1(3e33085f00b758acfc78034dc9a75fd6921fc3fe) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.7v", 0x00000,  0x400,  CRC(a1267336) SHA1(d11ea9d78526ac3c0dc6e57a2da5914273ad1e3f) )	/* Priority  Unused */
ROM_END

ROM_START( cninja0 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gn-02.rom", 0x00000, 0x20000, CRC(ccc59524) SHA1(430ae28ca38ec6a97b00cc3dee02d57e073819d4) )
	ROM_LOAD16_BYTE( "gn-05.rom", 0x00001, 0x20000, CRC(a002cbe4) SHA1(76f57e49fc41a779856f70feb14432a8ffd08bff) )
	ROM_LOAD16_BYTE( "gn-01.rom", 0x40000, 0x20000, CRC(18f0527c) SHA1(17b7ea68909c7c8b819578e2039f5be4a640ea75) )
	ROM_LOAD16_BYTE( "gn-04.rom", 0x40001, 0x20000, CRC(ea4b6d53) SHA1(263319750524756319587b6e51dfead0265809cb) )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000, CRC(0b110b16) SHA1(a967c8aeae3f0cee1f354583cf26ee736636aaf8) )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000, CRC(1e28e697) SHA1(2313e97f3a34892dfdc338944c0f00538fcae800) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  CRC(ca8bef96) SHA1(fcdbd598c85e339a3389a2ef58cf2d5b3a2779af) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gl-08.rom",  0x00001,  0x10000,  CRC(33a2b400) SHA1(fdb8de315f33705719c0ac03a61fb56ffbfdf597) )	/* chars */
	ROM_LOAD16_BYTE( "gl-09.rom",  0x00000,  0x10000,  CRC(5a2d4752) SHA1(617dd10a99b5b55ca64dcdd22a0f133b0d6b770d) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  CRC(de89c69a) SHA1(b41bdf859854b5541c7eae7cd541b910cea1f839) )	/* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  CRC(a8f05d33) SHA1(a1330bc9ca4648219403db087622badfc632b47d) )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  CRC(5b399eed) SHA1(490f8f9c0c557b0ba94c6019e3fe680641a0787e) )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  CRC(2220eb9f) SHA1(bdf0bd6e6ba375f0770b9d08a7efa32201cbb6ef) )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  CRC(56a53254) SHA1(10940cfdc6fbe9013865107de3394ca7f782d9c7) )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  CRC(144b94cc) SHA1(d982508608942a714b428a2b721bf24e1627cbb6) )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  CRC(82d44749) SHA1(c471fa573e00c2f8ae44068439ba6d849a124c68) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  CRC(d92e519d) SHA1(08238f12bf7058a3965ab6348b468e3d35d4cd23) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  CRC(08eb5264) SHA1(3e33085f00b758acfc78034dc9a75fd6921fc3fe) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.7v", 0x00000,  0x400,  CRC(a1267336) SHA1(d11ea9d78526ac3c0dc6e57a2da5914273ad1e3f) )	/* Priority  Unused */
ROM_END

ROM_START( cninjau )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gm02-3.1k", 0x00000, 0x20000, CRC(d931c3b1) SHA1(336390072a3a085fc534d9e2443c76104093b24f) )
	ROM_LOAD16_BYTE( "gm05-2.3k", 0x00001, 0x20000, CRC(7417d3fb) SHA1(24c65101585955d56440b63a307021b5c137d7b9) )
	ROM_LOAD16_BYTE( "gm01-2.1j", 0x40000, 0x20000, CRC(72041f7e) SHA1(cad62d6f3d77e361c7bb642401544baf01aec40d) )
	ROM_LOAD16_BYTE( "gm04-2.3j", 0x40001, 0x20000, CRC(2104d005) SHA1(7fcb33745f1200024a05feb87a35b82de6030bd2) )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000, CRC(0b110b16) SHA1(a967c8aeae3f0cee1f354583cf26ee736636aaf8) )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000, CRC(1e28e697) SHA1(2313e97f3a34892dfdc338944c0f00538fcae800) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  CRC(ca8bef96) SHA1(fcdbd598c85e339a3389a2ef58cf2d5b3a2779af) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gl-08.rom",  0x00001,  0x10000,  CRC(33a2b400) SHA1(fdb8de315f33705719c0ac03a61fb56ffbfdf597) )	/* chars */
	ROM_LOAD16_BYTE( "gl-09.rom",  0x00000,  0x10000,  CRC(5a2d4752) SHA1(617dd10a99b5b55ca64dcdd22a0f133b0d6b770d) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  CRC(de89c69a) SHA1(b41bdf859854b5541c7eae7cd541b910cea1f839) )	/* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  CRC(a8f05d33) SHA1(a1330bc9ca4648219403db087622badfc632b47d) )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  CRC(5b399eed) SHA1(490f8f9c0c557b0ba94c6019e3fe680641a0787e) )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  CRC(2220eb9f) SHA1(bdf0bd6e6ba375f0770b9d08a7efa32201cbb6ef) )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  CRC(56a53254) SHA1(10940cfdc6fbe9013865107de3394ca7f782d9c7) )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  CRC(144b94cc) SHA1(d982508608942a714b428a2b721bf24e1627cbb6) )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  CRC(82d44749) SHA1(c471fa573e00c2f8ae44068439ba6d849a124c68) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  CRC(d92e519d) SHA1(08238f12bf7058a3965ab6348b468e3d35d4cd23) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  CRC(08eb5264) SHA1(3e33085f00b758acfc78034dc9a75fd6921fc3fe) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.7v", 0x00000,  0x400,  CRC(a1267336) SHA1(d11ea9d78526ac3c0dc6e57a2da5914273ad1e3f) )	/* Priority  Unused */
ROM_END

ROM_START( joemac )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gl02-2.k1", 0x00000, 0x20000,  CRC(80da12e2) SHA1(1037ed56c15dbe1eb8bb8b70f0bc3affc1119782) )
	ROM_LOAD16_BYTE( "gl05-2.k3", 0x00001, 0x20000,  CRC(fe4dbbbb) SHA1(85a3c5470270ebfc695fc5e937cf133a33860bec) )
	ROM_LOAD16_BYTE( "gl01-2.j1", 0x40000, 0x20000,  CRC(0b245307) SHA1(839735c0739cebb7ac5e328aa8b69170f390b96e) )
	ROM_LOAD16_BYTE( "gl04-2.j3", 0x40001, 0x20000,  CRC(1b331f61) SHA1(7811c3c25bd17188ae9cc792e106b303ccb14cde) )
	ROM_LOAD16_BYTE( "gn-00.rom", 0x80000, 0x20000,  CRC(0b110b16) SHA1(a967c8aeae3f0cee1f354583cf26ee736636aaf8) )
	ROM_LOAD16_BYTE( "gn-03.rom", 0x80001, 0x20000,  CRC(1e28e697) SHA1(2313e97f3a34892dfdc338944c0f00538fcae800) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gl-07.rom",  0x00000,  0x10000,  CRC(ca8bef96) SHA1(fcdbd598c85e339a3389a2ef58cf2d5b3a2779af) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gl-08.rom",  0x00001,  0x10000,  CRC(33a2b400) SHA1(fdb8de315f33705719c0ac03a61fb56ffbfdf597) )	/* chars */
	ROM_LOAD16_BYTE( "gl-09.rom",  0x00000,  0x10000,  CRC(5a2d4752) SHA1(617dd10a99b5b55ca64dcdd22a0f133b0d6b770d) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  CRC(de89c69a) SHA1(b41bdf859854b5541c7eae7cd541b910cea1f839) )	/* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  CRC(a8f05d33) SHA1(a1330bc9ca4648219403db087622badfc632b47d) )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  CRC(5b399eed) SHA1(490f8f9c0c557b0ba94c6019e3fe680641a0787e) )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  CRC(2220eb9f) SHA1(bdf0bd6e6ba375f0770b9d08a7efa32201cbb6ef) )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  CRC(56a53254) SHA1(10940cfdc6fbe9013865107de3394ca7f782d9c7) )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  CRC(144b94cc) SHA1(d982508608942a714b428a2b721bf24e1627cbb6) )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  CRC(82d44749) SHA1(c471fa573e00c2f8ae44068439ba6d849a124c68) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gl-06.rom",  0x00000,  0x20000,  CRC(d92e519d) SHA1(08238f12bf7058a3965ab6348b468e3d35d4cd23) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mag-07.rom", 0x00000,  0x80000,  CRC(08eb5264) SHA1(3e33085f00b758acfc78034dc9a75fd6921fc3fe) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.7v", 0x00000,  0x400,  CRC(a1267336) SHA1(d11ea9d78526ac3c0dc6e57a2da5914273ad1e3f) )	/* Priority  Unused */
ROM_END

ROM_START( stoneage )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "sa_1_019.bin", 0x00000, 0x20000,  CRC(7fb8c44f) SHA1(0167805793a4288f545c0a8ea66bd1ad82bac437) )
	ROM_LOAD16_BYTE( "sa_1_033.bin", 0x00001, 0x20000,  CRC(961c752b) SHA1(b9ac7882662f84de7309c46f8c9344693215d9f7) )
	ROM_LOAD16_BYTE( "sa_1_018.bin", 0x40000, 0x20000,  CRC(a4043022) SHA1(084e80eaf4ffd9243996615ed20b7debcd185754) )
	ROM_LOAD16_BYTE( "sa_1_032.bin", 0x40001, 0x20000,  CRC(f52a3286) SHA1(04bc64ddefd1c52c87fe653423fb1e15746b8abc) )
	ROM_LOAD16_BYTE( "sa_1_017.bin", 0x80000, 0x20000,  CRC(08d6397a) SHA1(ae3a50a043b3247545378611381c593b3ceeb561) )
	ROM_LOAD16_BYTE( "sa_1_031.bin", 0x80001, 0x20000,  CRC(103079f5) SHA1(7ed28ab957be14974badeaa23f570f99ada61633) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "sa_1_012.bin",  0x00000,  0x10000, CRC(56058934) SHA1(99a007884c92c2d931d9270c6c2ec02fbc913922) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gl-08.rom",  0x00001,  0x10000,  CRC(33a2b400) SHA1(fdb8de315f33705719c0ac03a61fb56ffbfdf597) )	/* chars */
	ROM_LOAD16_BYTE( "gl-09.rom",  0x00000,  0x10000,  CRC(5a2d4752) SHA1(617dd10a99b5b55ca64dcdd22a0f133b0d6b770d) )

	/* The bootleg graphics are stored in a different arrangement but
		seem to be the same as the original set */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-02.rom", 0x000000, 0x80000,  CRC(de89c69a) SHA1(b41bdf859854b5541c7eae7cd541b910cea1f839) )	/* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mag-00.rom", 0x000000, 0x40000,  CRC(a8f05d33) SHA1(a1330bc9ca4648219403db087622badfc632b47d) )	/* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "mag-01.rom", 0x040000, 0x40000,  CRC(5b399eed) SHA1(490f8f9c0c557b0ba94c6019e3fe680641a0787e) )	/* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mag-03.rom", 0x000000, 0x80000,  CRC(2220eb9f) SHA1(bdf0bd6e6ba375f0770b9d08a7efa32201cbb6ef) )	/* sprites */
	ROM_LOAD16_BYTE( "mag-05.rom", 0x000001, 0x80000,  CRC(56a53254) SHA1(10940cfdc6fbe9013865107de3394ca7f782d9c7) )
	ROM_LOAD16_BYTE( "mag-04.rom", 0x100000, 0x80000,  CRC(144b94cc) SHA1(d982508608942a714b428a2b721bf24e1627cbb6) )
	ROM_LOAD16_BYTE( "mag-06.rom", 0x100001, 0x80000,  CRC(82d44749) SHA1(c471fa573e00c2f8ae44068439ba6d849a124c68) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "sa_1_069.bin",  0x00000,  0x40000, CRC(2188f3ca) SHA1(9c29b62ed261e63d701ff8d43020089c89a64ab2) )

	/* No extra Oki samples in the bootleg */
ROM_END

ROM_START( edrandy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
  	ROM_LOAD16_BYTE( "gg-00-2", 0x00000, 0x20000, CRC(ce1ba964) SHA1(da21734721344eff41a64a7f2382d5c027a24782) )
	ROM_LOAD16_BYTE( "gg-04-2", 0x00001, 0x20000, CRC(24caed19) SHA1(bdca689dbb13685e71d3385a9ff7b356d2459d45) )
	ROM_LOAD16_BYTE( "gg-01-2", 0x40000, 0x20000, CRC(33677b80) SHA1(d16b926053a61723d321a50f5cabf3e5faebadcf) )
 	ROM_LOAD16_BYTE( "gg-05-2", 0x40001, 0x20000, CRC(79a68ca6) SHA1(b1ec168ffe7aace481055a8f38d88ed71994191d) )
	ROM_LOAD16_BYTE( "ge-02",   0x80000, 0x20000, CRC(c2969fbb) SHA1(faa7da7f5271108dbbc95d111caa2c986e494933) )
	ROM_LOAD16_BYTE( "ge-06",   0x80001, 0x20000, CRC(5c2e6418) SHA1(b9ed769b27c37959fcba2acd6dba02ccd62149e7) )
	ROM_LOAD16_BYTE( "ge-03",   0xc0000, 0x20000, CRC(5e7b19a8) SHA1(637945e36c3665c74d31f4b14e600e93ed9be054) )
	ROM_LOAD16_BYTE( "ge-07",   0xc0001, 0x20000, CRC(5eb819a1) SHA1(1852cb624eccd0a424d404bc853b5df307875cc9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ge-09",    0x00000, 0x10000, CRC(9f94c60b) SHA1(56edf63850189b2168c602e1f21492ef14662682) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
 	ROM_LOAD16_BYTE( "gg-10",    0x000001, 0x10000, CRC(b96c6cbe) SHA1(1f3a18387f360705d2f2ab8f5780a270621e107f) )
	ROM_LOAD16_BYTE( "gg-11",    0x000000, 0x10000, CRC(ee567448) SHA1(40c673535b9edf7b8bbb4912235bbb09ef77e221) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-00",   0x000000, 0x40000, CRC(3735b22d) SHA1(fd9c3dc7a880592104c091730b9016641043987a) ) /* tiles 1 */
	ROM_CONTINUE(         0x080000, 0x40000 )
	ROM_LOAD( "mad-01",   0x040000, 0x40000, CRC(7bb13e1c) SHA1(2753e0345b746bb4e8a5572d057d0b888487cbc3) ) /* tiles 2 */
	ROM_CONTINUE(         0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-02",   0x000000, 0x80000, CRC(6c76face) SHA1(e485b118e1e5bdf130c7ae29eea2f192f85f93a5) ) /* tiles 3 */

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mad-03",   0x000000, 0x80000, CRC(c0bff892) SHA1(a3f3bdcc68f6183031438c0572e1e7c2c6fafb6a) ) /* sprites */
	ROM_LOAD16_BYTE( "mad-05",   0x000001, 0x80000, CRC(3f2ccf95) SHA1(ed9585f2162ca53a7621b86a9def45a46cd08331) )
	ROM_LOAD16_BYTE( "mad-04",   0x100000, 0x80000, CRC(464f3eb9) SHA1(c86212f37e4ca97ef71680643487f1e2afb7ec8a) )
	ROM_LOAD16_BYTE( "mad-06",   0x100001, 0x80000, CRC(60871f77) SHA1(45d3042986fba76951438fd69545a2a48e478a6a) )
	ROM_LOAD16_BYTE( "mad-07",   0x200000, 0x80000, CRC(ac03466e) SHA1(bfaa779f1818d8cd2b7de7a6ad1c2c396ce7309e) )
	ROM_LOAD16_BYTE( "mad-08",   0x200001, 0x80000, CRC(1b420ec8) SHA1(291119e16121fc81f982216772dc6d8bb3b3b12d) )
	ROM_LOAD16_BYTE( "mad-10",   0x300000, 0x80000, CRC(42da8ef0) SHA1(704a154db952e89c13a1bd115bdb57d5a6da479a) )
	ROM_LOAD16_BYTE( "mad-11",   0x300001, 0x80000, CRC(03c1f982) SHA1(1cc63e4e96356d1d281b254c3b7de009866e865b) )
	ROM_LOAD16_BYTE( "mad-09",   0x400000, 0x80000, CRC(930f4900) SHA1(dd09d3c8a251b8397996f6a3330e6e704f65d7fa) )
	ROM_LOAD16_BYTE( "mad-12",   0x400001, 0x80000, CRC(a0bd62b6) SHA1(a1ab365f3c63fc4edc32b09ecf8f982beb4bfae5) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ge-08",    0x00000, 0x20000, CRC(dfe28c7b) SHA1(aba55834b276cbab194e03858564077cad21eff1) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mad-13", 0x00000, 0x80000, CRC(6ab28eba) SHA1(12d3025478ee5af4bdea037656d9b1146cd9759f) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "ge-12", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority  Unused, same as Robocop 2 */
ROM_END

ROM_START( edrandy1 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
  	ROM_LOAD16_BYTE( "gg00-1.bin", 0x00000, 0x20000, CRC(a029cc4a) SHA1(3801fd6df6d1299972eeadbdbba1b0b7acf89139) )
  	ROM_LOAD16_BYTE( "gg04-1.bin", 0x00001, 0x20000, CRC(8b7928a4) SHA1(4075713a830c9d5e324bb790468ec555fa747106) )
	ROM_LOAD16_BYTE( "gg01-1.bin", 0x40000, 0x20000, CRC(84360123) SHA1(3e9241cf68839c15d7a1209fe735b51ed90a1de7) )
 	ROM_LOAD16_BYTE( "gg05-1.bin", 0x40001, 0x20000, CRC(0bf85d9d) SHA1(7b7c1c32d3f0de7e675cea3d2ba4f28e9ce387a9) )
	ROM_LOAD16_BYTE( "ge-02",   0x80000, 0x20000, CRC(c2969fbb) SHA1(faa7da7f5271108dbbc95d111caa2c986e494933) )
	ROM_LOAD16_BYTE( "ge-06",   0x80001, 0x20000, CRC(5c2e6418) SHA1(b9ed769b27c37959fcba2acd6dba02ccd62149e7) )
	ROM_LOAD16_BYTE( "ge-03",   0xc0000, 0x20000, CRC(5e7b19a8) SHA1(637945e36c3665c74d31f4b14e600e93ed9be054) )
	ROM_LOAD16_BYTE( "ge-07",   0xc0001, 0x20000, CRC(5eb819a1) SHA1(1852cb624eccd0a424d404bc853b5df307875cc9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ge-09",    0x00000, 0x10000, CRC(9f94c60b) SHA1(56edf63850189b2168c602e1f21492ef14662682) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
 	ROM_LOAD16_BYTE( "gg-10",    0x000001, 0x10000, CRC(b96c6cbe) SHA1(1f3a18387f360705d2f2ab8f5780a270621e107f) )
	ROM_LOAD16_BYTE( "gg-11",    0x000000, 0x10000, CRC(ee567448) SHA1(40c673535b9edf7b8bbb4912235bbb09ef77e221) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-00",   0x000000, 0x40000, CRC(3735b22d) SHA1(fd9c3dc7a880592104c091730b9016641043987a) ) /* tiles 1 */
	ROM_CONTINUE(         0x080000, 0x40000 )
	ROM_LOAD( "mad-01",   0x040000, 0x40000, CRC(7bb13e1c) SHA1(2753e0345b746bb4e8a5572d057d0b888487cbc3) ) /* tiles 2 */
	ROM_CONTINUE(         0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-02",   0x000000, 0x80000, CRC(6c76face) SHA1(e485b118e1e5bdf130c7ae29eea2f192f85f93a5) ) /* tiles 3 */

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mad-03",   0x000000, 0x80000, CRC(c0bff892) SHA1(a3f3bdcc68f6183031438c0572e1e7c2c6fafb6a) ) /* sprites */
	ROM_LOAD16_BYTE( "mad-05",   0x000001, 0x80000, CRC(3f2ccf95) SHA1(ed9585f2162ca53a7621b86a9def45a46cd08331) )
	ROM_LOAD16_BYTE( "mad-04",   0x100000, 0x80000, CRC(464f3eb9) SHA1(c86212f37e4ca97ef71680643487f1e2afb7ec8a) )
	ROM_LOAD16_BYTE( "mad-06",   0x100001, 0x80000, CRC(60871f77) SHA1(45d3042986fba76951438fd69545a2a48e478a6a) )
	ROM_LOAD16_BYTE( "mad-07",   0x200000, 0x80000, CRC(ac03466e) SHA1(bfaa779f1818d8cd2b7de7a6ad1c2c396ce7309e) )
	ROM_LOAD16_BYTE( "mad-08",   0x200001, 0x80000, CRC(1b420ec8) SHA1(291119e16121fc81f982216772dc6d8bb3b3b12d) )
	ROM_LOAD16_BYTE( "mad-10",   0x300000, 0x80000, CRC(42da8ef0) SHA1(704a154db952e89c13a1bd115bdb57d5a6da479a) )
	ROM_LOAD16_BYTE( "mad-11",   0x300001, 0x80000, CRC(03c1f982) SHA1(1cc63e4e96356d1d281b254c3b7de009866e865b) )
	ROM_LOAD16_BYTE( "mad-09",   0x400000, 0x80000, CRC(930f4900) SHA1(dd09d3c8a251b8397996f6a3330e6e704f65d7fa) )
	ROM_LOAD16_BYTE( "mad-12",   0x400001, 0x80000, CRC(a0bd62b6) SHA1(a1ab365f3c63fc4edc32b09ecf8f982beb4bfae5) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ge-08",    0x00000, 0x20000, CRC(dfe28c7b) SHA1(aba55834b276cbab194e03858564077cad21eff1) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mad-13", 0x00000, 0x80000, CRC(6ab28eba) SHA1(12d3025478ee5af4bdea037656d9b1146cd9759f) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "ge-12", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority  Unused, same as Robocop 2 */
ROM_END

ROM_START( edrandyj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
  	ROM_LOAD16_BYTE( "ge-00-2",   0x00000, 0x20000, CRC(b3d2403c) SHA1(9747dbe7905e1453e3e7764c874c523c54970e2e) )
  	ROM_LOAD16_BYTE( "ge-04-2",   0x00001, 0x20000, CRC(8a9624d6) SHA1(d5a9b56bc8a1d67fa28df95299cb205e9c965310) )
	ROM_LOAD16_BYTE( "ge-01-2",   0x40000, 0x20000, CRC(84360123) SHA1(3e9241cf68839c15d7a1209fe735b51ed90a1de7) )
 	ROM_LOAD16_BYTE( "ge-05-2",   0x40001, 0x20000, CRC(0bf85d9d) SHA1(7b7c1c32d3f0de7e675cea3d2ba4f28e9ce387a9) )
	ROM_LOAD16_BYTE( "ge-02",     0x80000, 0x20000, CRC(c2969fbb) SHA1(faa7da7f5271108dbbc95d111caa2c986e494933) )
	ROM_LOAD16_BYTE( "ge-06",     0x80001, 0x20000, CRC(5c2e6418) SHA1(b9ed769b27c37959fcba2acd6dba02ccd62149e7) )
	ROM_LOAD16_BYTE( "ge-03",     0xc0000, 0x20000, CRC(5e7b19a8) SHA1(637945e36c3665c74d31f4b14e600e93ed9be054) )
	ROM_LOAD16_BYTE( "ge-07",     0xc0001, 0x20000, CRC(5eb819a1) SHA1(1852cb624eccd0a424d404bc853b5df307875cc9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "ge-09",    0x00000, 0x10000, CRC(9f94c60b) SHA1(56edf63850189b2168c602e1f21492ef14662682) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ge-10",    0x000001, 0x10000, CRC(2528d795) SHA1(8081b5d13875287a75f868a0566a2d06e0e42949) )
  	ROM_LOAD16_BYTE( "ge-11",    0x000000, 0x10000, CRC(e34a931e) SHA1(0e06359347e48d53ee96d6551d34685110b0f5fb) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-00",   0x000000, 0x40000, CRC(3735b22d) SHA1(fd9c3dc7a880592104c091730b9016641043987a) ) /* tiles 1 */
	ROM_CONTINUE(         0x080000, 0x40000 )
	ROM_LOAD( "mad-01",   0x040000, 0x40000, CRC(7bb13e1c) SHA1(2753e0345b746bb4e8a5572d057d0b888487cbc3) ) /* tiles 2 */
	ROM_CONTINUE(         0x0c0000, 0x40000 )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mad-02",   0x000000, 0x80000, CRC(6c76face) SHA1(e485b118e1e5bdf130c7ae29eea2f192f85f93a5) ) /* tiles 3 */

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mad-03",   0x000000, 0x80000, CRC(c0bff892) SHA1(a3f3bdcc68f6183031438c0572e1e7c2c6fafb6a) ) /* sprites */
	ROM_LOAD16_BYTE( "mad-05",   0x000001, 0x80000, CRC(3f2ccf95) SHA1(ed9585f2162ca53a7621b86a9def45a46cd08331) )
	ROM_LOAD16_BYTE( "mad-04",   0x100000, 0x80000, CRC(464f3eb9) SHA1(c86212f37e4ca97ef71680643487f1e2afb7ec8a) )
	ROM_LOAD16_BYTE( "mad-06",   0x100001, 0x80000, CRC(60871f77) SHA1(45d3042986fba76951438fd69545a2a48e478a6a) )
	ROM_LOAD16_BYTE( "mad-07",   0x200000, 0x80000, CRC(ac03466e) SHA1(bfaa779f1818d8cd2b7de7a6ad1c2c396ce7309e) )
	ROM_LOAD16_BYTE( "mad-08",   0x200001, 0x80000, CRC(1b420ec8) SHA1(291119e16121fc81f982216772dc6d8bb3b3b12d) )
	ROM_LOAD16_BYTE( "mad-10",   0x300000, 0x80000, CRC(42da8ef0) SHA1(704a154db952e89c13a1bd115bdb57d5a6da479a) )
	ROM_LOAD16_BYTE( "mad-11",   0x300001, 0x80000, CRC(03c1f982) SHA1(1cc63e4e96356d1d281b254c3b7de009866e865b) )
	ROM_LOAD16_BYTE( "mad-09",   0x400000, 0x80000, CRC(930f4900) SHA1(dd09d3c8a251b8397996f6a3330e6e704f65d7fa) )
	ROM_LOAD16_BYTE( "mad-12",   0x400001, 0x80000, CRC(a0bd62b6) SHA1(a1ab365f3c63fc4edc32b09ecf8f982beb4bfae5) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ge-08",    0x00000, 0x20000, CRC(dfe28c7b) SHA1(aba55834b276cbab194e03858564077cad21eff1) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mad-13", 0x00000, 0x80000, CRC(6ab28eba) SHA1(12d3025478ee5af4bdea037656d9b1146cd9759f) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "ge-12", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority Unused, same as Robocop 2 */
ROM_END

ROM_START( robocop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "gq-03.1k",   0x00000, 0x20000, CRC(a7e90c28) SHA1(e1ff720e4e63de3adc94505a566e7340f65567d5) )
	ROM_LOAD16_BYTE( "gq-07.3k",   0x00001, 0x20000, CRC(d2287ec1) SHA1(8f596205c69b0ed3974cb0bd17fcc3b3bf47a0ca) )
	ROM_LOAD16_BYTE( "gq-02.1j",   0x40000, 0x20000, CRC(6777b8a0) SHA1(9081bd187c3b5923efab3e4abde952e9ab29d946) )
	ROM_LOAD16_BYTE( "gq-06.3j",   0x40001, 0x20000, CRC(e11e27b5) SHA1(03570da040b7cef2cecebce51b27f8a8fcf62eb1) )
	ROM_LOAD16_BYTE( "go-01-1.h1", 0x80000, 0x20000, CRC(ab5356c0) SHA1(297a89b4d9212c916745997bbb959b0ed660f909) )
	ROM_LOAD16_BYTE( "go-05-1.h3", 0x80001, 0x20000, CRC(ce21bda5) SHA1(615701d4abdb56d50da44589e6e03909f4b28d45) )
	ROM_LOAD16_BYTE( "go-00.f1",   0xc0000, 0x20000, CRC(a93369ea) SHA1(9e13c36112eb7ebc97dc919e24d0b2955c57e10e) )
	ROM_LOAD16_BYTE( "go-04.f3",   0xc0001, 0x20000, CRC(ee2f6ad9) SHA1(3abc07792f444a3415fd32e50d6855843e900b1d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gp-09.k13",  0x00000,  0x10000,  CRC(4a4e0f8d) SHA1(5408465667d2854bbade23a26ba619d42a0c22f8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gp10-1.y6",  0x00001,  0x10000,  CRC(d25d719c) SHA1(be874cf403ec0e607eb9b54b7cfff0a53f4d59a2) )	/* chars */
	ROM_LOAD16_BYTE( "gp11-1.z6",  0x00000,  0x10000,  CRC(030ded47) SHA1(59ded540b2601ec37255e871e38ac71a47c8d007) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-04.z4", 0x000000, 0x40000,  CRC(9b6ca18c) SHA1(29a20200ea50b9e8e79da072c1b6e580e6ca180f) )
	ROM_CONTINUE(          0x080000, 0x40000 )
	ROM_LOAD( "mah-03.y4", 0x040000, 0x40000,  CRC(37894ddc) SHA1(ee08440b3b2023ec6ee2af6d509b642bcead2e60) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-01.z1", 0x000000, 0x40000,  CRC(26e0dfff) SHA1(8cca2dbcda64f4bc6ee0842486da7dc7df3046fd) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )
	ROM_LOAD( "mah-00.y1", 0x040000, 0x40000,  CRC(7bd69e41) SHA1(296adbf7d40f1092bf38599b3bad51f39d8093b2) )
	ROM_CONTINUE(          0x100000, 0x40000 )
	ROM_LOAD( "mah-02.a1", 0x080000, 0x40000,  CRC(328a247d) SHA1(879f75452dc7c327fd5b35c960c58bc0c0efd33c) )
	ROM_CONTINUE(          0x140000, 0x40000 )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mah-05.y9",  0x000000, 0x80000,  CRC(6773e613) SHA1(ee6cb4272bb9f80e0d918dc059b40e0a47db0876) )
	ROM_LOAD16_BYTE( "mah-08.y12", 0x000001, 0x80000,  CRC(88d310a5) SHA1(82d249f331f51b7c72f9114ecf4e835ccdae7e97) )
	ROM_LOAD16_BYTE( "mah-06.z9",  0x100000, 0x80000,  CRC(27a8808a) SHA1(cb14992d1073de38406e36f5884d77933dd6b765) )
	ROM_LOAD16_BYTE( "mah-09.z12", 0x100001, 0x80000,  CRC(a58c43a7) SHA1(0b7f743cf0443d998479b7d5d95b8f2aaf1ef136) )
	ROM_LOAD16_BYTE( "mah-07.a9",  0x200000, 0x80000,  CRC(526f4190) SHA1(23cb79230ec267b8e4236381b5a596d7af8ec5b3) )
	ROM_LOAD16_BYTE( "mah-10.a12", 0x200001, 0x80000,  CRC(14b770da) SHA1(6d57da630da1ec457ebaeed8c251e85bd737e97c) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gp-08.j13",  0x00000,  0x20000,  CRC(365183b1) SHA1(7d3c201c49981c3ac84022283b048e380cbb7ec3) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mah-11.f13", 0x00000,  0x80000,  CRC(642bc692) SHA1(8d9e446b7633bb6acc46d9f92044a69b99a0ccc9) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "go-12.v7", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority  Unused */
ROM_END

ROM_START( robocp2u )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "robo03.k1",  0x00000, 0x20000, CRC(f4c96cc9) SHA1(2eb58aca1134c33f2084267e65a565f9adc6ba49) )
	ROM_LOAD16_BYTE( "robo07.k3",  0x00001, 0x20000, CRC(11e53a7c) SHA1(cdeb7f1983a771238d9d2000f99aed35ae4a06ee) )
	ROM_LOAD16_BYTE( "robo02.j1",  0x40000, 0x20000, CRC(fa086a0d) SHA1(34a3f9c6890e1fbacbde3e39a861e42d511cd8ec) )
	ROM_LOAD16_BYTE( "robo06.j3",  0x40001, 0x20000, CRC(703b49d0) SHA1(be51644fe730d0cb95e1b09f8595da2e36c09aeb) )
	ROM_LOAD16_BYTE( "go-01-1.h1", 0x80000, 0x20000, CRC(ab5356c0) SHA1(297a89b4d9212c916745997bbb959b0ed660f909) )
	ROM_LOAD16_BYTE( "go-05-1.h3", 0x80001, 0x20000, CRC(ce21bda5) SHA1(615701d4abdb56d50da44589e6e03909f4b28d45) )
	ROM_LOAD16_BYTE( "go-00.f1",   0xc0000, 0x20000, CRC(a93369ea) SHA1(9e13c36112eb7ebc97dc919e24d0b2955c57e10e) )
	ROM_LOAD16_BYTE( "go-04.f3",   0xc0001, 0x20000, CRC(ee2f6ad9) SHA1(3abc07792f444a3415fd32e50d6855843e900b1d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gp-09.k13",  0x00000,  0x10000,  CRC(4a4e0f8d) SHA1(5408465667d2854bbade23a26ba619d42a0c22f8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gp10-1.y6",  0x00001,  0x10000,  CRC(d25d719c) SHA1(be874cf403ec0e607eb9b54b7cfff0a53f4d59a2) )	/* chars */
	ROM_LOAD16_BYTE( "gp11-1.z6",  0x00000,  0x10000,  CRC(030ded47) SHA1(59ded540b2601ec37255e871e38ac71a47c8d007) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-04.z4", 0x000000, 0x40000,  CRC(9b6ca18c) SHA1(29a20200ea50b9e8e79da072c1b6e580e6ca180f) )
	ROM_CONTINUE(          0x080000, 0x40000 )
	ROM_LOAD( "mah-03.y4", 0x040000, 0x40000,  CRC(37894ddc) SHA1(ee08440b3b2023ec6ee2af6d509b642bcead2e60) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-01.z1", 0x000000, 0x40000,  CRC(26e0dfff) SHA1(8cca2dbcda64f4bc6ee0842486da7dc7df3046fd) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )
	ROM_LOAD( "mah-00.y1", 0x040000, 0x40000,  CRC(7bd69e41) SHA1(296adbf7d40f1092bf38599b3bad51f39d8093b2) )
	ROM_CONTINUE(          0x100000, 0x40000 )
	ROM_LOAD( "mah-02.a1", 0x080000, 0x40000,  CRC(328a247d) SHA1(879f75452dc7c327fd5b35c960c58bc0c0efd33c) )
	ROM_CONTINUE(          0x140000, 0x40000 )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mah-05.y9",  0x000000, 0x80000,  CRC(6773e613) SHA1(ee6cb4272bb9f80e0d918dc059b40e0a47db0876) )
	ROM_LOAD16_BYTE( "mah-08.y12", 0x000001, 0x80000,  CRC(88d310a5) SHA1(82d249f331f51b7c72f9114ecf4e835ccdae7e97) )
	ROM_LOAD16_BYTE( "mah-06.z9",  0x100000, 0x80000,  CRC(27a8808a) SHA1(cb14992d1073de38406e36f5884d77933dd6b765) )
	ROM_LOAD16_BYTE( "mah-09.z12", 0x100001, 0x80000,  CRC(a58c43a7) SHA1(0b7f743cf0443d998479b7d5d95b8f2aaf1ef136) )
	ROM_LOAD16_BYTE( "mah-07.a9",  0x200000, 0x80000,  CRC(526f4190) SHA1(23cb79230ec267b8e4236381b5a596d7af8ec5b3) )
	ROM_LOAD16_BYTE( "mah-10.a12", 0x200001, 0x80000,  CRC(14b770da) SHA1(6d57da630da1ec457ebaeed8c251e85bd737e97c) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gp-08.j13",  0x00000,  0x20000,  CRC(365183b1) SHA1(7d3c201c49981c3ac84022283b048e380cbb7ec3) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mah-11.f13", 0x00000,  0x80000,  CRC(642bc692) SHA1(8d9e446b7633bb6acc46d9f92044a69b99a0ccc9) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "go-12.v7", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority  Unused */
ROM_END

ROM_START( robocp2j )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "go_03-1.k1", 0x00000, 0x20000, CRC(52506608) SHA1(a0e738fe1083a17cb40f28ad95b695b6caebf3b1) )
	ROM_LOAD16_BYTE( "go_07-1.k3", 0x00001, 0x20000, CRC(739cda17) SHA1(5a69873d79beabace4739ad313e8c090919206ba) )
	ROM_LOAD16_BYTE( "go_02-1.j1", 0x40000, 0x20000, CRC(48c0ace9) SHA1(cf53eb97552aa503e62eb3361af4a19494dfe1ff) )
	ROM_LOAD16_BYTE( "go_06-1.j3", 0x40001, 0x20000, CRC(41abec87) SHA1(83d24d9344508124a8ced402bdc5749e5fcc8e9c) )
	ROM_LOAD16_BYTE( "go-01-1.h1", 0x80000, 0x20000, CRC(ab5356c0) SHA1(297a89b4d9212c916745997bbb959b0ed660f909) )
	ROM_LOAD16_BYTE( "go-05-1.h3", 0x80001, 0x20000, CRC(ce21bda5) SHA1(615701d4abdb56d50da44589e6e03909f4b28d45) )
	ROM_LOAD16_BYTE( "go-00.f1",   0xc0000, 0x20000, CRC(a93369ea) SHA1(9e13c36112eb7ebc97dc919e24d0b2955c57e10e) )
	ROM_LOAD16_BYTE( "go-04.f3",   0xc0001, 0x20000, CRC(ee2f6ad9) SHA1(3abc07792f444a3415fd32e50d6855843e900b1d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "gp-09.k13",  0x00000,  0x10000,  CRC(4a4e0f8d) SHA1(5408465667d2854bbade23a26ba619d42a0c22f8) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gp10-1.y6",  0x00001,  0x10000,  CRC(d25d719c) SHA1(be874cf403ec0e607eb9b54b7cfff0a53f4d59a2) )	/* chars */
	ROM_LOAD16_BYTE( "gp11-1.z6",  0x00000,  0x10000,  CRC(030ded47) SHA1(59ded540b2601ec37255e871e38ac71a47c8d007) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-04.z4", 0x000000, 0x40000,  CRC(9b6ca18c) SHA1(29a20200ea50b9e8e79da072c1b6e580e6ca180f) )
	ROM_CONTINUE(          0x080000, 0x40000 )
	ROM_LOAD( "mah-03.y4", 0x040000, 0x40000,  CRC(37894ddc) SHA1(ee08440b3b2023ec6ee2af6d509b642bcead2e60) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mah-01.z1", 0x000000, 0x40000,  CRC(26e0dfff) SHA1(8cca2dbcda64f4bc6ee0842486da7dc7df3046fd) )
	ROM_CONTINUE(          0x0c0000, 0x40000 )
	ROM_LOAD( "mah-00.y1", 0x040000, 0x40000,  CRC(7bd69e41) SHA1(296adbf7d40f1092bf38599b3bad51f39d8093b2) )
	ROM_CONTINUE(          0x100000, 0x40000 )
	ROM_LOAD( "mah-02.a1", 0x080000, 0x40000,  CRC(328a247d) SHA1(879f75452dc7c327fd5b35c960c58bc0c0efd33c) )
	ROM_CONTINUE(          0x140000, 0x40000 )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mah-05.y9",  0x000000, 0x80000,  CRC(6773e613) SHA1(ee6cb4272bb9f80e0d918dc059b40e0a47db0876) )
	ROM_LOAD16_BYTE( "mah-08.y12", 0x000001, 0x80000,  CRC(88d310a5) SHA1(82d249f331f51b7c72f9114ecf4e835ccdae7e97) )
	ROM_LOAD16_BYTE( "mah-06.z9",  0x100000, 0x80000,  CRC(27a8808a) SHA1(cb14992d1073de38406e36f5884d77933dd6b765) )
	ROM_LOAD16_BYTE( "mah-09.z12", 0x100001, 0x80000,  CRC(a58c43a7) SHA1(0b7f743cf0443d998479b7d5d95b8f2aaf1ef136) )
	ROM_LOAD16_BYTE( "mah-07.a9",  0x200000, 0x80000,  CRC(526f4190) SHA1(23cb79230ec267b8e4236381b5a596d7af8ec5b3) )
	ROM_LOAD16_BYTE( "mah-10.a12", 0x200001, 0x80000,  CRC(14b770da) SHA1(6d57da630da1ec457ebaeed8c251e85bd737e97c) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gp-08.j13",  0x00000,  0x20000,  CRC(365183b1) SHA1(7d3c201c49981c3ac84022283b048e380cbb7ec3) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "mah-11.f13", 0x00000,  0x80000,  CRC(642bc692) SHA1(8d9e446b7633bb6acc46d9f92044a69b99a0ccc9) )	/* banked */

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "go-12.v7", 0x00000,  0x400,  CRC(278f674f) SHA1(d4f5b9770d6d2ddebf1b999e291c80a3e274d492) )	/* Priority  Unused */
ROM_END

ROM_START( mutantf )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hd03-4.2c",  0x00000, 0x20000, CRC(94859545) SHA1(4b218442bf1ba01b9b6b54c0037c76c827b79d35) )
	ROM_LOAD16_BYTE("hd00-4.2a",  0x00001, 0x20000, CRC(3cdb648f) SHA1(f803d2894d4c32de770861c70f837377afd329fe) )
	ROM_LOAD16_BYTE("hf-04-1.4c", 0x40000, 0x20000, CRC(fd2ea8d7) SHA1(00e50d42fcc7f6d5076963b02f5abb36275dc993) )
	ROM_LOAD16_BYTE("hf-01-1.4a", 0x40001, 0x20000, CRC(48a247ac) SHA1(c69d67e44ffae92b261de247f8d3eac2e02fcf11) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "hf-12.21j",  0x00000,  0x10000,  CRC(13d55f11) SHA1(6438dca57f43b3ca6d273bf82b62104a49260132) )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD16_BYTE( "hf-06-1.8d", 0x000000, 0x10000, CRC(8b7a558b) SHA1(06f1f6910b9a54e208ee9a0b734d5df946016236) )
	ROM_LOAD16_BYTE( "hf-07-1.9d", 0x000001, 0x10000, CRC(d2a3d449) SHA1(10ad634eb0238f6e9ba04266e4dbaf9470f5d169) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-00.8a", 0x000000, 0x80000,  CRC(e56f528d) SHA1(3908d9b189fa4895c532d1d1f133df0913810cf9) ) /* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-01.9a",  0x000000, 0x40000,  CRC(c3d5173d) SHA1(2b6559bf65d7cc5b957ad347b64cf6a18f661686) ) /* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "maf-02.11a", 0x040000, 0x40000,  CRC(0b37d849) SHA1(a0606fb8130a2e86a241ce5ce0b4f61373a88c17) ) /* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_BYTE( "maf-03.18a",   0x000000, 0x100000, CRC(f4366d2c) SHA1(20964d0e1b879b3e5cb5d18a46d2a17dca2b4171) )
	ROM_LOAD16_BYTE( "maf-04.20a",   0x200000, 0x100000, CRC(0c8f654e) SHA1(e566d4b789b345e20caf7e061e43be7c2e1be9b2) )
	ROM_LOAD16_BYTE( "maf-05.21a",   0x400000, 0x080000, CRC(b0cfeb80) SHA1(b8519c604b03eb8bcf26d00a43b39d48f1b45ab5) )
	ROM_LOAD16_BYTE( "maf-06.18d",   0x000001, 0x100000, CRC(f5c7a9b5) SHA1(92efc9401347598c90acf62c9aef30109c990ad6) )
	ROM_LOAD16_BYTE( "maf-07.20d",   0x200001, 0x100000, CRC(fd6008a3) SHA1(7b680424eca3804c70fa0c4dc415d665c8626498) )
	ROM_LOAD16_BYTE( "maf-08.21d",   0x400001, 0x080000, CRC(e41cf1e7) SHA1(06524e1aed0adc4c32c92e16a00dc983014f4994) )

	ROM_REGION( 0x40000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites 2 */
	ROM_LOAD32_BYTE("hf-08.15a", 0x00001, 0x10000, CRC(93b7279f) SHA1(14304a1ffe1bc791bfa83f8200793d897449133c) )
	ROM_LOAD32_BYTE("hf-09.17a", 0x00003, 0x10000, CRC(05e2c074) SHA1(ec95303e8196424864964b5d2ae862bf75571e83) )
	ROM_LOAD32_BYTE("hf-10.15c", 0x00000, 0x10000, CRC(9b06f418) SHA1(d1579ae36676e38c96ee55a1ffa20aa307a21654) )
	ROM_LOAD32_BYTE("hf-11.17c", 0x00002, 0x10000, CRC(3859a531) SHA1(a2a0c1aa28181b5ef6c075ff0118178340389693) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-10.20l",    0x00000, 0x40000, CRC(7c57f48b) SHA1(9a5624553b3b038d70f9b517f410a635c00a8771) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-09.18l",    0x00000, 0x80000, CRC(28e7ed81) SHA1(e168a2748b75c647f6f9c0d7d25d4f046aa98094) )
ROM_END

ROM_START( mutantfa )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hd03-3.2c",   0x00000, 0x20000, CRC(e6f53574) SHA1(98d5a76bda52346e4bee5b1b0755e3fee4ad8283) )
	ROM_LOAD16_BYTE("hd00-3.2a",   0x00001, 0x20000, CRC(d3055454) SHA1(83531ae52e5928ac64279bcb98878eef291f8f70) )
	ROM_LOAD16_BYTE("hf-04-1.4c", 0x40000, 0x20000, CRC(fd2ea8d7) SHA1(00e50d42fcc7f6d5076963b02f5abb36275dc993) )
	ROM_LOAD16_BYTE("hf-01-1.4a", 0x40001, 0x20000, CRC(48a247ac) SHA1(c69d67e44ffae92b261de247f8d3eac2e02fcf11) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "hf-12.21j",  0x00000,  0x10000,  CRC(13d55f11) SHA1(6438dca57f43b3ca6d273bf82b62104a49260132) )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD16_BYTE( "hf-06-1.8d", 0x000000, 0x10000, CRC(8b7a558b) SHA1(06f1f6910b9a54e208ee9a0b734d5df946016236) )
	ROM_LOAD16_BYTE( "hf-07-1.9d", 0x000001, 0x10000, CRC(d2a3d449) SHA1(10ad634eb0238f6e9ba04266e4dbaf9470f5d169) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-00.8a", 0x000000, 0x80000,  CRC(e56f528d) SHA1(3908d9b189fa4895c532d1d1f133df0913810cf9) ) /* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-01.9a",  0x000000, 0x40000,  CRC(c3d5173d) SHA1(2b6559bf65d7cc5b957ad347b64cf6a18f661686) ) /* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "maf-02.11a", 0x040000, 0x40000,  CRC(0b37d849) SHA1(a0606fb8130a2e86a241ce5ce0b4f61373a88c17) ) /* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_BYTE( "maf-03.18a",   0x000000, 0x100000, CRC(f4366d2c) SHA1(20964d0e1b879b3e5cb5d18a46d2a17dca2b4171) )
	ROM_LOAD16_BYTE( "maf-04.20a",   0x200000, 0x100000, CRC(0c8f654e) SHA1(e566d4b789b345e20caf7e061e43be7c2e1be9b2) )
	ROM_LOAD16_BYTE( "maf-05.21a",   0x400000, 0x080000, CRC(b0cfeb80) SHA1(b8519c604b03eb8bcf26d00a43b39d48f1b45ab5) )
	ROM_LOAD16_BYTE( "maf-06.18d",   0x000001, 0x100000, CRC(f5c7a9b5) SHA1(92efc9401347598c90acf62c9aef30109c990ad6) )
	ROM_LOAD16_BYTE( "maf-07.20d",   0x200001, 0x100000, CRC(fd6008a3) SHA1(7b680424eca3804c70fa0c4dc415d665c8626498) )
	ROM_LOAD16_BYTE( "maf-08.21d",   0x400001, 0x080000, CRC(e41cf1e7) SHA1(06524e1aed0adc4c32c92e16a00dc983014f4994) )

	ROM_REGION( 0x40000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites 2 */
	ROM_LOAD32_BYTE("hf-08.15a", 0x00001, 0x10000, CRC(93b7279f) SHA1(14304a1ffe1bc791bfa83f8200793d897449133c) )
	ROM_LOAD32_BYTE("hf-09.17a", 0x00003, 0x10000, CRC(05e2c074) SHA1(ec95303e8196424864964b5d2ae862bf75571e83) )
	ROM_LOAD32_BYTE("hf-10.15c", 0x00000, 0x10000, CRC(9b06f418) SHA1(d1579ae36676e38c96ee55a1ffa20aa307a21654) )
	ROM_LOAD32_BYTE("hf-11.17c", 0x00002, 0x10000, CRC(3859a531) SHA1(a2a0c1aa28181b5ef6c075ff0118178340389693) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-10.20l",    0x00000, 0x40000, CRC(7c57f48b) SHA1(9a5624553b3b038d70f9b517f410a635c00a8771) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-09.18l",    0x00000, 0x80000, CRC(28e7ed81) SHA1(e168a2748b75c647f6f9c0d7d25d4f046aa98094) )
ROM_END

ROM_START( deathbrd )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hf-03-2.2c", 0x00000, 0x20000, CRC(fb86fff3) SHA1(af4cfc19ec85e0aa49b5e46d95bdd94a20922cce) )
	ROM_LOAD16_BYTE("hf-00-2.2a", 0x00001, 0x20000, CRC(099aa422) SHA1(b62f261b1903dd2d1a308f7abb9584b3726204b5) )
	ROM_LOAD16_BYTE("hf-04-1.4c", 0x40000, 0x20000, CRC(fd2ea8d7) SHA1(00e50d42fcc7f6d5076963b02f5abb36275dc993) )
	ROM_LOAD16_BYTE("hf-01-1.4a", 0x40001, 0x20000, CRC(48a247ac) SHA1(c69d67e44ffae92b261de247f8d3eac2e02fcf11) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "hf-12.21j",  0x00000,  0x10000,  CRC(13d55f11) SHA1(6438dca57f43b3ca6d273bf82b62104a49260132) )

	ROM_REGION( 0x0a0000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD16_BYTE( "hf-06-1.8d", 0x000000, 0x10000, CRC(8b7a558b) SHA1(06f1f6910b9a54e208ee9a0b734d5df946016236) )
	ROM_LOAD16_BYTE( "hf-07-1.9d", 0x000001, 0x10000, CRC(d2a3d449) SHA1(10ad634eb0238f6e9ba04266e4dbaf9470f5d169) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-00.8a", 0x000000, 0x80000,  CRC(e56f528d) SHA1(3908d9b189fa4895c532d1d1f133df0913810cf9) ) /* tiles 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maf-01.9a",  0x000000, 0x40000,  CRC(c3d5173d) SHA1(2b6559bf65d7cc5b957ad347b64cf6a18f661686) ) /* tiles 1 */
	ROM_CONTINUE(           0x080000, 0x40000 )
	ROM_LOAD( "maf-02.11a", 0x040000, 0x40000,  CRC(0b37d849) SHA1(a0606fb8130a2e86a241ce5ce0b4f61373a88c17) ) /* tiles 2 */
	ROM_CONTINUE(           0x0c0000, 0x40000 )

	ROM_REGION( 0x500000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_BYTE( "maf-03.18a",   0x000000, 0x100000, CRC(f4366d2c) SHA1(20964d0e1b879b3e5cb5d18a46d2a17dca2b4171) )
	ROM_LOAD16_BYTE( "maf-04.20a",   0x200000, 0x100000, CRC(0c8f654e) SHA1(e566d4b789b345e20caf7e061e43be7c2e1be9b2) )
	ROM_LOAD16_BYTE( "maf-05.21a",   0x400000, 0x080000, CRC(b0cfeb80) SHA1(b8519c604b03eb8bcf26d00a43b39d48f1b45ab5) )
	ROM_LOAD16_BYTE( "maf-06.18d",   0x000001, 0x100000, CRC(f5c7a9b5) SHA1(92efc9401347598c90acf62c9aef30109c990ad6) )
	ROM_LOAD16_BYTE( "maf-07.20d",   0x200001, 0x100000, CRC(fd6008a3) SHA1(7b680424eca3804c70fa0c4dc415d665c8626498) )
	ROM_LOAD16_BYTE( "maf-08.21d",   0x400001, 0x080000, CRC(e41cf1e7) SHA1(06524e1aed0adc4c32c92e16a00dc983014f4994) )

	ROM_REGION( 0x40000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites 2 */
	ROM_LOAD32_BYTE("hf-08.15a", 0x00001, 0x10000, CRC(93b7279f) SHA1(14304a1ffe1bc791bfa83f8200793d897449133c) )
	ROM_LOAD32_BYTE("hf-09.17a", 0x00003, 0x10000, CRC(05e2c074) SHA1(ec95303e8196424864964b5d2ae862bf75571e83) )
	ROM_LOAD32_BYTE("hf-10.15c", 0x00000, 0x10000, CRC(9b06f418) SHA1(d1579ae36676e38c96ee55a1ffa20aa307a21654) )
	ROM_LOAD32_BYTE("hf-11.17c", 0x00002, 0x10000, CRC(3859a531) SHA1(a2a0c1aa28181b5ef6c075ff0118178340389693) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-10.20l",    0x00000, 0x40000, CRC(7c57f48b) SHA1(9a5624553b3b038d70f9b517f410a635c00a8771) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* ADPCM samples */
	ROM_LOAD( "maf-09.18l",    0x00000, 0x80000, CRC(28e7ed81) SHA1(e168a2748b75c647f6f9c0d7d25d4f046aa98094) )
ROM_END

/**********************************************************************************/

static void cninja_patch(void)
{
	data16_t *RAM = (UINT16 *)memory_region(REGION_CPU1);
	int i;

	for (i=0; i<0x80000/2; i++) {
		int aword=RAM[i];

		if (aword==0x66ff || aword==0x67ff) {
			data16_t doublecheck=RAM[i-4];

			/* Cmpi + btst controlling opcodes */
			if (doublecheck==0xc39 || doublecheck==0x839) {
				RAM[i]=0x4E71;
		        RAM[i-1]=0x4E71;
		        RAM[i-2]=0x4E71;
		        RAM[i-3]=0x4E71;
		        RAM[i-4]=0x4E71;
			}
		}
	}
}

/**********************************************************************************/

static DRIVER_INIT( cninja )
{
	install_mem_write16_handler(0, 0x1bc0a8, 0x1bc0a9, cninja_sound_w);
	cninja_patch();
}

static DRIVER_INIT( stoneage )
{
	install_mem_write16_handler(0, 0x1bc0a8, 0x1bc0a9, stoneage_sound_w);
}

static DRIVER_INIT( mutantf )
{
	const data8_t *src = memory_region(REGION_GFX2);
	data8_t *dst = memory_region(REGION_GFX1);

	/* The 16x16 graphic has some 8x8 chars in it - decode them in GFX1 */
	memcpy(dst+0x50000,dst+0x10000,0x10000);
	memcpy(dst+0x10000,src,0x40000);
	memcpy(dst+0x60000,src+0x40000,0x40000);

	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
}

/**********************************************************************************/

GAME( 1990, edrandy,  0,       edrandy,  edrandy, 0,        ROT0, "Data East Corporation", "The Cliffhanger - Edward Randy (World revision 2)" )
GAME( 1990, edrandy1, edrandy, edrandy,  edrandy, 0,        ROT0, "Data East Corporation", "The Cliffhanger - Edward Randy (World revision 1)" )
GAME( 1990, edrandyj, edrandy, edrandy,  edrandy, 0,        ROT0, "Data East Corporation", "The Cliffhanger - Edward Randy (Japan)" )
GAME( 1991, cninja,   0,       cninja,   cninja,  cninja,   ROT0, "Data East Corporation", "Caveman Ninja (World revision 3)" )
GAME( 1991, cninja0,  cninja,  cninja,   cninja,  cninja,   ROT0, "Data East Corporation", "Caveman Ninja (World revision 0)" )
GAME( 1991, cninjau,  cninja,  cninja,   cninjau, cninja,   ROT0, "Data East Corporation", "Caveman Ninja (US)" )
GAME( 1991, joemac,   cninja,  cninja,   cninja,  cninja,   ROT0, "Data East Corporation", "Tatakae Genshizin Joe and Mac (Japan)" )
GAME( 1991, stoneage, cninja,  stoneage, cninja,  stoneage, ROT0, "bootleg", "Stoneage" )
GAME( 1991, robocop2, 0,       robocop2, robocop2,0,        ROT0, "Data East Corporation", "Robocop 2 (World)" )
GAME( 1991, robocp2u, robocop2,robocop2, robocop2,0,        ROT0, "Data East Corporation", "Robocop 2 (US)" )
GAME( 1991, robocp2j, robocop2,robocop2, robocop2,0,        ROT0, "Data East Corporation", "Robocop 2 (Japan)" )
GAME( 1992, mutantf,  0,       mutantf,  mutantf, mutantf,  ROT0, "Data East Corporation", "Mutant Fighter (World Rev 4, EM-5)" )
GAME( 1992, mutantfa, mutantf, mutantf,  mutantf, mutantf,  ROT0, "Data East Corporation", "Mutant Fighter (World Rev 3, EM-4)" )
GAME( 1992, deathbrd, mutantf, mutantf,  mutantf, mutantf,  ROT0, "Data East Corporation", "Death Brade (Japan Rev 2, JM-3)" )
