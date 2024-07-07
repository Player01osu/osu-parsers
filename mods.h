#ifndef MODS_H
#define MODS_H

/* https://github.com/ppy/osu-api/wiki#mods */
enum {
	MOD_NONE              = 0,
	MOD_NOFAIL            = 1 << 0,
	MOD_EASY              = 1 << 1,
	MOD_TOUCHDEVICE       = 1 << 2,
	MOD_HIDDEN            = 1 << 3,
	MOD_HARDROCK          = 1 << 4,
	MOD_SUDDENDEATH       = 1 << 5,
	MOD_DOUBLETIME        = 1 << 6,
	MOD_RELAX             = 1 << 7,
	MOD_HALFTIME          = 1 << 8,
	MOD_NIGHTCORE         = 1 << 9,   /* Only set along with DoubleTime. i.e: NC only gives 576 */
	MOD_FLASHLIGHT        = 1 << 10,
	MOD_AUTOPLAY          = 1 << 11,
	MOD_SPUNOUT           = 1 << 12,
	MOD_RELAX2            = 1 << 13, /* Autopilot */
	MOD_PERFECT           = 1 << 14, /* Only set along with SuddenDeath. i.e: PF only gives 16416 */
	MOD_KEY4              = 1 << 15,
	MOD_KEY5              = 1 << 16,
	MOD_KEY6              = 1 << 17,
	MOD_KEY7              = 1 << 18,
	MOD_KEY8              = 1 << 19,
	MOD_FADEIN            = 1 << 20,
	MOD_RANDOM            = 1 << 21,
	MOD_CINEMA            = 1 << 22,
	MOD_TARGET            = 1 << 23,
	MOD_KEY9              = 1 << 24,
	MOD_KEYCOOP           = 1 << 25,
	MOD_KEY1              = 1 << 26,
	MOD_KEY3              = 1 << 27,
	MOD_KEY2              = 1 << 28,
	MOD_SCOREV2           = 1 << 29,
	MOD_MIRROR            = 1 << 30,
	MOD_KEYMOD            = MOD_KEY1 | MOD_KEY2 | MOD_KEY3 | MOD_KEY4 | MOD_KEY5 | MOD_KEY6 | MOD_KEY7 | MOD_KEY8 | MOD_KEY9 | MOD_KEYCOOP,
	MOD_FREEMODALLOWED    = MOD_NOFAIL | MOD_EASY | MOD_HIDDEN | MOD_HARDROCK | MOD_SUDDENDEATH | MOD_FLASHLIGHT | MOD_FADEIN | MOD_RELAX | MOD_RELAX2 | MOD_SPUNOUT | MOD_KEYMOD,
	MOD_SCOREINCREASEMODS = MOD_HIDDEN | MOD_HARDROCK | MOD_DOUBLETIME | MOD_FLASHLIGHT | MOD_FADEIN
};

#endif
