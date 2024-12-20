#if 0
void gensub( WORD *buf, LONG base, LONG step, LONG con ) {
	WORD	i,x;

	for( i=0; i<256; i++ ) {
		x = base/con;
		if ( base%con<0 )
			x -= 1;
		*buf++ = x;
		base += step;
	}
}

void gentabs( WORD *tab ) {
	/* BT.601 YUV for 160 levels per sample */
	/* 16*r=[y]+[v1] */
	/* 16*g=[y]+[v2]+[u2] */
	/* 16*b=[y]+[u1] */
	gensub( tab + 0x000, -258561, 4080, 160 );	/* [y] */
	gensub( tab + 0x100, -595840, 4655, 160 );	/* [v1] */
	gensub( tab + 0x200, -1060608, 8286, 160 );	/* [u1] */
	gensub( tab + 0x300, 0x4A200, -2372, 160 );	/* [v2] */
	gensub( tab + 0x400, 0x32800, -1616, 160 );	/* [u2] */
}
#endif

/*  tables for real is WORD, but compiler has problem */
static uint16_t const tables[] = {
	0xF9AF,0xF9C9,0xF9E2,0xF9FC,0xFA15,0xFA2F,0xFA48,0xFA62,
	0xFA7B,0xFA95,0xFAAE,0xFAC8,0xFAE1,0xFAFB,0xFB14,0xFB2E,
	0xFB47,0xFB61,0xFB7A,0xFB94,0xFBAD,0xFBC7,0xFBE0,0xFBFA,
	0xFC13,0xFC2D,0xFC46,0xFC60,0xFC79,0xFC93,0xFCAC,0xFCC6,
	0xFCDF,0xFCF9,0xFD12,0xFD2C,0xFD45,0xFD5F,0xFD78,0xFD92,
	0xFDAB,0xFDC5,0xFDDE,0xFDF8,0xFE11,0xFE2B,0xFE44,0xFE5E,
	0xFE77,0xFE91,0xFEAA,0xFEC4,0xFEDD,0xFEF7,0xFF10,0xFF2A,
	0xFF43,0xFF5D,0xFF76,0xFF90,0xFFA9,0xFFC3,0xFFDC,0xFFF6,
	0x000F,0x0029,0x0042,0x005C,0x0075,0x008F,0x00A8,0x00C2,
	0x00DB,0x00F5,0x010E,0x0128,0x0141,0x015B,0x0174,0x018E,
	0x01A7,0x01C1,0x01DA,0x01F4,0x020D,0x0227,0x0240,0x025A,
	0x0273,0x028D,0x02A6,0x02C0,0x02D9,0x02F3,0x030C,0x0326,
	0x033F,0x0359,0x0372,0x038C,0x03A5,0x03BF,0x03D8,0x03F2,
	0x040B,0x0425,0x043E,0x0458,0x0471,0x048B,0x04A4,0x04BE,
	0x04D7,0x04F1,0x050A,0x0524,0x053D,0x0557,0x0570,0x058A,
	0x05A3,0x05BD,0x05D6,0x05F0,0x0609,0x0623,0x063C,0x0656,
	0x066F,0x0689,0x06A2,0x06BC,0x06D5,0x06EF,0x0708,0x0722,
	0x073B,0x0755,0x076E,0x0788,0x07A1,0x07BB,0x07D4,0x07EE,
	0x0807,0x0821,0x083A,0x0854,0x086D,0x0887,0x08A0,0x08BA,
	0x08D3,0x08ED,0x0906,0x0920,0x0939,0x0953,0x096C,0x0986,
	0x099F,0x09B9,0x09D2,0x09EC,0x0A05,0x0A1F,0x0A38,0x0A52,
	0x0A6B,0x0A85,0x0A9E,0x0AB8,0x0AD1,0x0AEB,0x0B04,0x0B1E,
	0x0B37,0x0B51,0x0B6A,0x0B84,0x0B9D,0x0BB7,0x0BD0,0x0BEA,
	0x0C03,0x0C1D,0x0C36,0x0C50,0x0C69,0x0C83,0x0C9C,0x0CB6,
	0x0CCF,0x0CE9,0x0D02,0x0D1C,0x0D35,0x0D4F,0x0D68,0x0D82,
	0x0D9B,0x0DB5,0x0DCE,0x0DE8,0x0E01,0x0E1B,0x0E34,0x0E4E,
	0x0E67,0x0E81,0x0E9A,0x0EB4,0x0ECD,0x0EE7,0x0F00,0x0F1A,
	0x0F33,0x0F4D,0x0F66,0x0F80,0x0F99,0x0FB3,0x0FCC,0x0FE6,
	0x0FFF,0x1019,0x1032,0x104C,0x1065,0x107F,0x1098,0x10B2,
	0x10CB,0x10E5,0x10FE,0x1118,0x1131,0x114B,0x1164,0x117E,
	0x1197,0x11B1,0x11CA,0x11E4,0x11FD,0x1217,0x1230,0x124A,
	0x1263,0x127D,0x1296,0x12B0,0x12C9,0x12E3,0x12FC,0x1316,

	0xF174,0xF191,0xF1AE,0xF1CB,0xF1E8,0xF205,0xF222,0xF23F,
	0xF25C,0xF279,0xF296,0xF2B4,0xF2D1,0xF2EE,0xF30B,0xF328,
	0xF345,0xF362,0xF37F,0xF39C,0xF3B9,0xF3D6,0xF3F4,0xF411,
	0xF42E,0xF44B,0xF468,0xF485,0xF4A2,0xF4BF,0xF4DC,0xF4F9,
	0xF517,0xF534,0xF551,0xF56E,0xF58B,0xF5A8,0xF5C5,0xF5E2,
	0xF5FF,0xF61C,0xF639,0xF657,0xF674,0xF691,0xF6AE,0xF6CB,
	0xF6E8,0xF705,0xF722,0xF73F,0xF75C,0xF779,0xF797,0xF7B4,
	0xF7D1,0xF7EE,0xF80B,0xF828,0xF845,0xF862,0xF87F,0xF89C,
	0xF8BA,0xF8D7,0xF8F4,0xF911,0xF92E,0xF94B,0xF968,0xF985,
	0xF9A2,0xF9BF,0xF9DC,0xF9FA,0xFA17,0xFA34,0xFA51,0xFA6E,
	0xFA8B,0xFAA8,0xFAC5,0xFAE2,0xFAFF,0xFB1C,0xFB3A,0xFB57,
	0xFB74,0xFB91,0xFBAE,0xFBCB,0xFBE8,0xFC05,0xFC22,0xFC3F,
	0xFC5D,0xFC7A,0xFC97,0xFCB4,0xFCD1,0xFCEE,0xFD0B,0xFD28,
	0xFD45,0xFD62,0xFD7F,0xFD9D,0xFDBA,0xFDD7,0xFDF4,0xFE11,
	0xFE2E,0xFE4B,0xFE68,0xFE85,0xFEA2,0xFEBF,0xFEDD,0xFEFA,
	0xFF17,0xFF34,0xFF51,0xFF6E,0xFF8B,0xFFA8,0xFFC5,0xFFE2,
/* 0x300 */
	0x0000,0x001D,0x003A,0x0057,0x0074,0x0091,0x00AE,0x00CB,
	0x00E8,0x0105,0x0122,0x0140,0x015D,0x017A,0x0197,0x01B4,
	0x01D1,0x01EE,0x020B,0x0228,0x0245,0x0262,0x0280,0x029D,
	0x02BA,0x02D7,0x02F4,0x0311,0x032E,0x034B,0x0368,0x0385,
	0x03A3,0x03C0,0x03DD,0x03FA,0x0417,0x0434,0x0451,0x046E,
	0x048B,0x04A8,0x04C5,0x04E3,0x0500,0x051D,0x053A,0x0557,
	0x0574,0x0591,0x05AE,0x05CB,0x05E8,0x0605,0x0623,0x0640,
	0x065D,0x067A,0x0697,0x06B4,0x06D1,0x06EE,0x070B,0x0728,
	0x0746,0x0763,0x0780,0x079D,0x07BA,0x07D7,0x07F4,0x0811,
	0x082E,0x084B,0x0868,0x0886,0x08A3,0x08C0,0x08DD,0x08FA,
	0x0917,0x0934,0x0951,0x096E,0x098B,0x09A8,0x09C6,0x09E3,
	0x0A00,0x0A1D,0x0A3A,0x0A57,0x0A74,0x0A91,0x0AAE,0x0ACB,
	0x0AE9,0x0B06,0x0B23,0x0B40,0x0B5D,0x0B7A,0x0B97,0x0BB4,
	0x0BD1,0x0BEE,0x0C0B,0x0C29,0x0C46,0x0C63,0x0C80,0x0C9D,
	0x0CBA,0x0CD7,0x0CF4,0x0D11,0x0D2E,0x0D4B,0x0D69,0x0D86,
	0x0DA3,0x0DC0,0x0DDD,0x0DFA,0x0E17,0x0E34,0x0E51,0x0E6E,

	0xE61B,0xE64E,0xE682,0xE6B6,0xE6EA,0xE71E,0xE751,0xE785,
	0xE7B9,0xE7ED,0xE821,0xE854,0xE888,0xE8BC,0xE8F0,0xE924,
	0xE957,0xE98B,0xE9BF,0xE9F3,0xEA26,0xEA5A,0xEA8E,0xEAC2,
	0xEAF6,0xEB29,0xEB5D,0xEB91,0xEBC5,0xEBF9,0xEC2C,0xEC60,
	0xEC94,0xECC8,0xECFB,0xED2F,0xED63,0xED97,0xEDCB,0xEDFE,
	0xEE32,0xEE66,0xEE9A,0xEECE,0xEF01,0xEF35,0xEF69,0xEF9D,
	0xEFD1,0xF004,0xF038,0xF06C,0xF0A0,0xF0D3,0xF107,0xF13B,
	0xF16F,0xF1A3,0xF1D6,0xF20A,0xF23E,0xF272,0xF2A6,0xF2D9,
	0xF30D,0xF341,0xF375,0xF3A8,0xF3DC,0xF410,0xF444,0xF478,
	0xF4AB,0xF4DF,0xF513,0xF547,0xF57B,0xF5AE,0xF5E2,0xF616,
	0xF64A,0xF67D,0xF6B1,0xF6E5,0xF719,0xF74D,0xF780,0xF7B4,
	0xF7E8,0xF81C,0xF850,0xF883,0xF8B7,0xF8EB,0xF91F,0xF953,
	0xF986,0xF9BA,0xF9EE,0xFA22,0xFA55,0xFA89,0xFABD,0xFAF1,
	0xFB25,0xFB58,0xFB8C,0xFBC0,0xFBF4,0xFC28,0xFC5B,0xFC8F,
	0xFCC3,0xFCF7,0xFD2A,0xFD5E,0xFD92,0xFDC6,0xFDFA,0xFE2D,
	0xFE61,0xFE95,0xFEC9,0xFEFD,0xFF30,0xFF64,0xFF98,0xFFCC,
/* 0x500 */
	0x0000,0x0033,0x0067,0x009B,0x00CF,0x0102,0x0136,0x016A,
	0x019E,0x01D2,0x0205,0x0239,0x026D,0x02A1,0x02D5,0x0308,
	0x033C,0x0370,0x03A4,0x03D7,0x040B,0x043F,0x0473,0x04A7,
	0x04DA,0x050E,0x0542,0x0576,0x05AA,0x05DD,0x0611,0x0645,
	0x0679,0x06AC,0x06E0,0x0714,0x0748,0x077C,0x07AF,0x07E3,
	0x0817,0x084B,0x087F,0x08B2,0x08E6,0x091A,0x094E,0x0982,
	0x09B5,0x09E9,0x0A1D,0x0A51,0x0A84,0x0AB8,0x0AEC,0x0B20,
	0x0B54,0x0B87,0x0BBB,0x0BEF,0x0C23,0x0C57,0x0C8A,0x0CBE,
	0x0CF2,0x0D26,0x0D59,0x0D8D,0x0DC1,0x0DF5,0x0E29,0x0E5C,
	0x0E90,0x0EC4,0x0EF8,0x0F2C,0x0F5F,0x0F93,0x0FC7,0x0FFB,
	0x102F,0x1062,0x1096,0x10CA,0x10FE,0x1131,0x1165,0x1199,
	0x11CD,0x1201,0x1234,0x1268,0x129C,0x12D0,0x1304,0x1337,
	0x136B,0x139F,0x13D3,0x1406,0x143A,0x146E,0x14A2,0x14D6,
	0x1509,0x153D,0x1571,0x15A5,0x15D9,0x160C,0x1640,0x1674,
	0x16A8,0x16DB,0x170F,0x1743,0x1777,0x17AB,0x17DE,0x1812,
	0x1846,0x187A,0x18AE,0x18E1,0x1915,0x1949,0x197D,0x19B1,

	0x0769,0x075A,0x074B,0x073D,0x072E,0x071F,0x0710,0x0701,
	0x06F3,0x06E4,0x06D5,0x06C6,0x06B7,0x06A8,0x069A,0x068B,
	0x067C,0x066D,0x065E,0x064F,0x0641,0x0632,0x0623,0x0614,
	0x0605,0x05F6,0x05E8,0x05D9,0x05CA,0x05BB,0x05AC,0x059E,
	0x058F,0x0580,0x0571,0x0562,0x0553,0x0545,0x0536,0x0527,
	0x0518,0x0509,0x04FA,0x04EC,0x04DD,0x04CE,0x04BF,0x04B0,
	0x04A2,0x0493,0x0484,0x0475,0x0466,0x0457,0x0449,0x043A,
	0x042B,0x041C,0x040D,0x03FE,0x03F0,0x03E1,0x03D2,0x03C3,
	0x03B4,0x03A5,0x0397,0x0388,0x0379,0x036A,0x035B,0x034D,
	0x033E,0x032F,0x0320,0x0311,0x0302,0x02F4,0x02E5,0x02D6,
	0x02C7,0x02B8,0x02A9,0x029B,0x028C,0x027D,0x026E,0x025F,
	0x0251,0x0242,0x0233,0x0224,0x0215,0x0206,0x01F8,0x01E9,
	0x01DA,0x01CB,0x01BC,0x01AD,0x019F,0x0190,0x0181,0x0172,
	0x0163,0x0154,0x0146,0x0137,0x0128,0x0119,0x010A,0x00FC,
	0x00ED,0x00DE,0x00CF,0x00C0,0x00B1,0x00A3,0x0094,0x0085,
	0x0076,0x0067,0x0058,0x004A,0x003B,0x002C,0x001D,0x000E,
/* 0x700 */
	0x0000,0xFFF1,0xFFE2,0xFFD3,0xFFC4,0xFFB5,0xFFA7,0xFF98,
	0xFF89,0xFF7A,0xFF6B,0xFF5C,0xFF4E,0xFF3F,0xFF30,0xFF21,
	0xFF12,0xFF03,0xFEF5,0xFEE6,0xFED7,0xFEC8,0xFEB9,0xFEAB,
	0xFE9C,0xFE8D,0xFE7E,0xFE6F,0xFE60,0xFE52,0xFE43,0xFE34,
	0xFE25,0xFE16,0xFE07,0xFDF9,0xFDEA,0xFDDB,0xFDCC,0xFDBD,
	0xFDAF,0xFDA0,0xFD91,0xFD82,0xFD73,0xFD64,0xFD56,0xFD47,
	0xFD38,0xFD29,0xFD1A,0xFD0B,0xFCFD,0xFCEE,0xFCDF,0xFCD0,
	0xFCC1,0xFCB2,0xFCA4,0xFC95,0xFC86,0xFC77,0xFC68,0xFC5A,
	0xFC4B,0xFC3C,0xFC2D,0xFC1E,0xFC0F,0xFC01,0xFBF2,0xFBE3,
	0xFBD4,0xFBC5,0xFBB6,0xFBA8,0xFB99,0xFB8A,0xFB7B,0xFB6C,
	0xFB5E,0xFB4F,0xFB40,0xFB31,0xFB22,0xFB13,0xFB05,0xFAF6,
	0xFAE7,0xFAD8,0xFAC9,0xFABA,0xFAAC,0xFA9D,0xFA8E,0xFA7F,
	0xFA70,0xFA61,0xFA53,0xFA44,0xFA35,0xFA26,0xFA17,0xFA09,
	0xF9FA,0xF9EB,0xF9DC,0xF9CD,0xF9BE,0xF9B0,0xF9A1,0xF992,
	0xF983,0xF974,0xF965,0xF957,0xF948,0xF939,0xF92A,0xF91B,
	0xF90D,0xF8FE,0xF8EF,0xF8E0,0xF8D1,0xF8C2,0xF8B4,0xF8A5,

	0x050C,0x0502,0x04F8,0x04EE,0x04E4,0x04DA,0x04D0,0x04C6,
	0x04BC,0x04B1,0x04A7,0x049D,0x0493,0x0489,0x047F,0x0475,
	0x046B,0x0461,0x0457,0x044C,0x0442,0x0438,0x042E,0x0424,
	0x041A,0x0410,0x0406,0x03FC,0x03F2,0x03E7,0x03DD,0x03D3,
	0x03C9,0x03BF,0x03B5,0x03AB,0x03A1,0x0397,0x038D,0x0382,
	0x0378,0x036E,0x0364,0x035A,0x0350,0x0346,0x033C,0x0332,
	0x0328,0x031D,0x0313,0x0309,0x02FF,0x02F5,0x02EB,0x02E1,
	0x02D7,0x02CD,0x02C3,0x02B8,0x02AE,0x02A4,0x029A,0x0290,
	0x0286,0x027C,0x0272,0x0268,0x025E,0x0253,0x0249,0x023F,
	0x0235,0x022B,0x0221,0x0217,0x020D,0x0203,0x01F9,0x01EE,
	0x01E4,0x01DA,0x01D0,0x01C6,0x01BC,0x01B2,0x01A8,0x019E,
	0x0194,0x0189,0x017F,0x0175,0x016B,0x0161,0x0157,0x014D,
	0x0143,0x0139,0x012F,0x0124,0x011A,0x0110,0x0106,0x00FC,
	0x00F2,0x00E8,0x00DE,0x00D4,0x00CA,0x00BF,0x00B5,0x00AB,
	0x00A1,0x0097,0x008D,0x0083,0x0079,0x006F,0x0065,0x005A,
	0x0050,0x0046,0x003C,0x0032,0x0028,0x001E,0x0014,0x000A,
/* 0x900 */
	0x0000,0xFFF5,0xFFEB,0xFFE1,0xFFD7,0xFFCD,0xFFC3,0xFFB9,
	0xFFAF,0xFFA5,0xFF9B,0xFF90,0xFF86,0xFF7C,0xFF72,0xFF68,
	0xFF5E,0xFF54,0xFF4A,0xFF40,0xFF36,0xFF2B,0xFF21,0xFF17,
	0xFF0D,0xFF03,0xFEF9,0xFEEF,0xFEE5,0xFEDB,0xFED1,0xFEC6,
	0xFEBC,0xFEB2,0xFEA8,0xFE9E,0xFE94,0xFE8A,0xFE80,0xFE76,
	0xFE6C,0xFE61,0xFE57,0xFE4D,0xFE43,0xFE39,0xFE2F,0xFE25,
	0xFE1B,0xFE11,0xFE07,0xFDFC,0xFDF2,0xFDE8,0xFDDE,0xFDD4,
	0xFDCA,0xFDC0,0xFDB6,0xFDAC,0xFDA2,0xFD97,0xFD8D,0xFD83,
	0xFD79,0xFD6F,0xFD65,0xFD5B,0xFD51,0xFD47,0xFD3D,0xFD32,
	0xFD28,0xFD1E,0xFD14,0xFD0A,0xFD00,0xFCF6,0xFCEC,0xFCE2,
	0xFCD8,0xFCCD,0xFCC3,0xFCB9,0xFCAF,0xFCA5,0xFC9B,0xFC91,
	0xFC87,0xFC7D,0xFC73,0xFC68,0xFC5E,0xFC54,0xFC4A,0xFC40,
	0xFC36,0xFC2C,0xFC22,0xFC18,0xFC0E,0xFC03,0xFBF9,0xFBEF,
	0xFBE5,0xFBDB,0xFBD1,0xFBC7,0xFBBD,0xFBB3,0xFBA9,0xFB9E,
	0xFB94,0xFB8A,0xFB80,0xFB76,0xFB6C,0xFB62,0xFB58,0xFB4E,
	0xFB44,0xFB39,0xFB2F,0xFB25,0xFB1B,0xFB11,0xFB07,0xFAFD
};

