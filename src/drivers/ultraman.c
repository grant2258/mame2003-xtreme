/***************************************************************************

Ultraman (c) 1991  Banpresto / Bandai

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "ost_samples.h"

/* from vidhrdw/ultraman.c */
WRITE16_HANDLER( ultraman_gfxctrl_w );
VIDEO_START( ultraman );
VIDEO_UPDATE( ultraman );

static READ16_HANDLER( ultraman_K051937_r )
{
	return K051937_r(offset);
}

static READ16_HANDLER( ultraman_K051960_r )
{
	return K051960_r(offset);
}

static READ16_HANDLER( ultraman_K051316_0_r )
{
	return K051316_0_r(offset);
}

static READ16_HANDLER( ultraman_K051316_1_r )
{
	return K051316_1_r(offset);
}

static READ16_HANDLER( ultraman_K051316_2_r )
{
	return K051316_2_r(offset);
}

static WRITE16_HANDLER( ultraman_K051316_0_w )
{
	if (ACCESSING_LSB)
		K051316_0_w(offset, data & 0xff);
		        }

static WRITE16_HANDLER( ultraman_K051316_1_w )
{
	if (ACCESSING_LSB)
		K051316_1_w(offset, data & 0xff);
}

static WRITE16_HANDLER( ultraman_K051316_2_w )
{
	if (ACCESSING_LSB)
		K051316_2_w(offset, data & 0xff);
}

static WRITE16_HANDLER( ultraman_K051316_ctrl_0_w )
{
	if (ACCESSING_LSB)
		K051316_ctrl_0_w(offset, data & 0xff);
}

static WRITE16_HANDLER( ultraman_K051316_ctrl_1_w )
{
	if (ACCESSING_LSB)
		K051316_ctrl_1_w(offset, data & 0xff);

}

static WRITE16_HANDLER( ultraman_K051316_ctrl_2_w )
{
	if (ACCESSING_LSB)
		K051316_ctrl_2_w(offset, data & 0xff);
}

static WRITE16_HANDLER( ultraman_K051937_w )
{
	if (ACCESSING_LSB)
		K051937_w(offset, data & 0xff);
}

static WRITE16_HANDLER( ultraman_K051960_w )
{
	if (ACCESSING_LSB)
		K051960_w(offset, data & 0xff);
}

static WRITE16_HANDLER( sound_cmd_w )
{

	if( ost_support_enabled(OST_SUPPORT_ULTRAMAN) )
	{

		if(generate_ost_sound( data ))
			if (ACCESSING_LSB)
				soundlatch_w(0,data & 0xff);
	}

	else
		if (ACCESSING_LSB)
			soundlatch_w(0,data & 0xff);

}

static WRITE16_HANDLER( sound_irq_trigger_w )
{
	if (ACCESSING_LSB)
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}



static MEMORY_READ16_START( ultraman_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },				/* ROM */
	{ 0x080000, 0x08ffff, MRA16_RAM },				/* RAM */
	{ 0x180000, 0x183fff, MRA16_RAM },				/* Palette */
	{ 0x1c0000, 0x1c0001, input_port_0_word_r },	/* Coins + Service */
	{ 0x1c0002, 0x1c0003, input_port_1_word_r },	/* 1P controls */
	{ 0x1c0004, 0x1c0005, input_port_2_word_r },	/* 2P controls */
	{ 0x1c0006, 0x1c0007, input_port_3_word_r },	/* DIPSW #1 */
	{ 0x1c0008, 0x1c0009, input_port_4_word_r },	/* DIPSW #2 */
	{ 0x204000, 0x204fff, ultraman_K051316_0_r },	/* K051316 #0 RAM */
	{ 0x205000, 0x205fff, ultraman_K051316_1_r },	/* K051316 #1 RAM */
	{ 0x206000, 0x206fff, ultraman_K051316_2_r },	/* K051316 #2 RAM */
	{ 0x304000, 0x30400f, ultraman_K051937_r },		/* Sprite control */
	{ 0x304800, 0x304fff, ultraman_K051960_r },		/* Sprite RAM */
MEMORY_END

static MEMORY_WRITE16_START( ultraman_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },					/* ROM */
	{ 0x080000, 0x08ffff, MWA16_RAM },					/* RAM */
	{ 0x180000, 0x183fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },/* Palette */
	{ 0x1c0018, 0x1c0019, ultraman_gfxctrl_w },	/* counters + gfx ctrl */
	{ 0x1c0020, 0x1c0021, sound_cmd_w },
	{ 0x1c0028, 0x1c0029, sound_irq_trigger_w },
	{ 0x1c0030, 0x1c0031, watchdog_reset16_w },
	{ 0x204000, 0x204fff, ultraman_K051316_0_w },		/* K051316 #0 RAM */
	{ 0x205000, 0x205fff, ultraman_K051316_1_w },		/* K051316 #1 RAM */
	{ 0x206000, 0x206fff, ultraman_K051316_2_w },		/* K051316 #2 RAM */
	{ 0x207f80, 0x207f9f, ultraman_K051316_ctrl_0_w	},	/* K051316 #0 registers  */
	{ 0x207fa0, 0x207fbf, ultraman_K051316_ctrl_1_w	},	/* K051316 #1 registers */
	{ 0x207fc0, 0x207fdf, ultraman_K051316_ctrl_2_w	},	/* K051316 #2 registers */
	{ 0x304000, 0x30400f, ultraman_K051937_w },			/* Sprite control */
	{ 0x304800, 0x304fff, ultraman_K051960_w },			/* Sprite RAM */
MEMORY_END

static MEMORY_READ_START( ultraman_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },					/* ROM */
	{ 0x8000, 0xbfff, MRA_RAM },					/* RAM */
	{ 0xc000, 0xc000, soundlatch_r },				/* Sound latch read */
	{ 0xe000, 0xe000, OKIM6295_status_0_r },		/* M6295 */
	{ 0xf001, 0xf001, YM2151_status_port_0_r },		/* YM2151 */
MEMORY_END

static MEMORY_WRITE_START( ultraman_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM */
	{ 0x8000, 0xbfff, MWA_RAM },					/* RAM */
/*	{ 0xd000, 0xd000, MWA_NOP },					 // ??? /*/
	{ 0xe000, 0xe000, OKIM6295_data_0_w },			/* M6295 */
	{ 0xf000, 0xf000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xf001, 0xf001, YM2151_data_port_0_w },		/* YM2151 */
MEMORY_END

static PORT_WRITE_START( ultraman_writeport_sound )

PORT_END


INPUT_PORTS_START( ultraman )

	PORT_START	/* Coins + Service */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN #1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START	/* IN #2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Allow Continue" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "Easy" )
	PORT_DIPSETTING(	0x30, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_DIPNAME( 0x40, 0x40, "Upright Controls" )
	PORT_DIPSETTING(	0x40, "Single" )
	PORT_DIPSETTING(	0x00, "Dual" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static struct YM2151interface ym2151_interface =
{
	1,
	24000000/6,	/* 4 MHz (tempo verified against real board) */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
};

static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 8000 },			/* 8KHz? */
	{ REGION_SOUND1 },	/* memory region */
    { 50 }
};



static MACHINE_DRIVER_START( ultraman )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)		/* 12 MHz? */
	MDRV_CPU_MEMORY(ultraman_readmem,ultraman_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,24000000/6)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 4 MHz? */
	MDRV_CPU_MEMORY(ultraman_readmem_sound,ultraman_writemem_sound)
	MDRV_CPU_PORTS(0,ultraman_writeport_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(ultraman)
	MDRV_VIDEO_UPDATE(ultraman)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_INSTALL_OST_SUPPORT(OST_SUPPORT_ULTRAMAN)
MACHINE_DRIVER_END



ROM_START( ultraman )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"910-b01.c11",	0x000000, 0x020000, CRC(3d9e4323) SHA1(54ee218c9be1ac029836624839d0845b39e6e30f) )
	ROM_LOAD16_BYTE(	"910-b02.d11",	0x000001, 0x020000, CRC(d24c82e9) SHA1(e792e2601e235939546fe98d52bfafe5a95b3491) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )	/* Z80 code */
	ROM_LOAD( "910-a05.d05",	0x00000, 0x08000, CRC(ebaef189) SHA1(73e6163466d55ae782f55839ba9c98f06c30876b) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Sprites */
	ROM_LOAD( "910-a19.l04",	0x000000, 0x080000, CRC(2dc9ffdc) SHA1(aa34247c82d48c8d13f5209be292127938a4a682) )
	ROM_LOAD( "910-a20.l01",	0x080000, 0x080000, CRC(a4298dce) SHA1(62faf8f0c0490a9562b75ce27909fbee6e84b22a) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 )	/* BG 1  */
	ROM_LOAD( "910-a07.j15",	0x000000, 0x020000, CRC(8b43a64e) SHA1(e373d0fd88b59fb01782dfaeccb1e13673a35766) )
	ROM_LOAD( "910-a08.j16",	0x020000, 0x020000, CRC(c3829826) SHA1(0d383a7afac2a3b5da692375a2b2cd675848861a) )
	ROM_LOAD( "910-a09.j18",	0x040000, 0x020000, CRC(ee10b519) SHA1(a34bd7d89bb8a19af7252ed96ffce212788c586b) )
	ROM_LOAD( "910-a10.j19",	0x060000, 0x020000, CRC(cffbb0c3) SHA1(e9ebe350289f0436de10a6289b04eed3b6a9f98e) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* BG 2 */
	ROM_LOAD( "910-a11.l15",	0x000000, 0x020000, CRC(17a5581d) SHA1(aca5d465a0e181a266a165aeb0112a4696b0cd18) )
	ROM_LOAD( "910-a12.l16",	0x020000, 0x020000, CRC(39763fb5) SHA1(0e1795af4bae545a0a2be265398837fb2d623232) )
	ROM_LOAD( "910-a13.l18",	0x040000, 0x020000, CRC(66b25a4f) SHA1(954552b005582c90d570ae32c715108ec4b088f1) )
	ROM_LOAD( "910-a14.l19",	0x060000, 0x020000, CRC(09fbd412) SHA1(d11587db7b03f3a75ad8964523bb34f4453bbaca) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* BG 3 */
	ROM_LOAD( "910-a15.m15",	0x000000, 0x020000, CRC(6d5bfbb7) SHA1(e98c594446b506cb32cc5cc958d2f0de22ebed5e) )
	ROM_LOAD( "910-a16.m16",	0x020000, 0x020000, CRC(5f6f8c3d) SHA1(e365836d2263f36aa4602f0618bf7ce693d2e106) )
	ROM_LOAD( "910-a17.m18",	0x040000, 0x020000, CRC(1f3ec4ff) SHA1(875f53516f47decc4ce31154cf4694c8429ee4ea) )
	ROM_LOAD( "910-a18.m19",	0x060000, 0x020000, CRC(fdc42929) SHA1(079827c1b1a3c32f8547dd91bba8ae37034c16be) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "910-a21.f14",	0x000000, 0x000100, CRC(64460fbc) SHA1(b5295e1d3303d5d816ad44da7b011bbfa613f9e4) )	/* priority encoder (not used) */

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* M6295 data */
	ROM_LOAD( "910-a06.c06",	0x000000, 0x040000, CRC(28fa99c9) SHA1(54663d79ee105ac18d6ba01333a52e3732f2e5fe) )
ROM_END



static DRIVER_INIT( ultraman )
{
	konami_rom_deinterleave_2(REGION_GFX1);
}


GAME( 1991, ultraman, 0, ultraman, ultraman, ultraman, ROT0, "Banpresto/Bandai", "Ultraman (Japan)" )
