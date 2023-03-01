#include <fmt/format.h>
#include <unordered_map>

#ifdef _DEBUG
#include "debug.h"
#endif
#include "engine/load_file.hpp"
#include "engine/trn.hpp"
#include "lighting.h"

namespace devilution {

uint8_t *GetCustomTRN(uint8_t pal8blue, int8_t pal8blueDimming, uint8_t pal8red, int8_t pal8redDimming, uint8_t pal8yellow, int8_t pal8yellowDimming, uint8_t pal8orange, int8_t pal8orangeDimming, uint8_t pal16beige, int8_t pal16beigeDimming, uint8_t pal16blue, int8_t pal16blueDimming, uint8_t pal16yellow, int8_t pal16yellowDimming, uint8_t pal16orange, int8_t pal16orangeDimming, uint8_t pal16red, int8_t pal16redDimming, uint8_t pal16gray, int8_t pal16grayDimming )
{
	std::unique_ptr<uint8_t[]> customTrn(new uint8_t[256]);
	// Remaining colors on each line of colors after the first index
	int pal8colors = 8;
	int pal16colors = 16;

	// Fill town/dungeon palette as default
	for (int i = 0; i < 128; i++) {
		customTrn[i] = i;
	}

	// Fill PAL8_BLUE indices
	for (int i = PAL8_BLUE; i < PAL8_RED; i++) {
		int value = i - PAL8_BLUE;
		value += pal8blue;
		value += pal8blueDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal8blue), pal8blue + pal8colors - 1);
	}

	// Fill PAL8_RED indices
	for (int i = PAL8_RED; i < PAL8_YELLOW; i++) {
		int value = i - PAL8_RED;
		value += pal8red;
		value += pal8redDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal8red), pal8red + pal8colors - 1);
	}

	// Fill PAL8_YELLOW indices
	for (int i = PAL8_YELLOW; i < PAL8_ORANGE; i++) {
		int value = i - PAL8_YELLOW;
		value += pal8yellow;
		value += pal8yellowDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal8yellow), pal8yellow + pal8colors - 1);
	}

	// Fill PAL8_ORANGE indices
	for (int i = PAL8_ORANGE; i < PAL16_BEIGE; i++) {
		int value = i - PAL8_ORANGE;
		value += pal8orange;
		value += pal8orangeDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal8orange), pal8orange + pal8colors - 1);
	}

	// Fill PAL16_BEIGE indices
	for (int i = PAL16_BEIGE; i < PAL16_BLUE; i++) {
		int value = i - PAL16_BEIGE;
		value += pal16beige;
		value += pal16beigeDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16beige), pal16beige + pal16colors - 1);
	}

	// Fill PAL16_BLUE indices
	for (int i = PAL16_BLUE; i < PAL16_YELLOW; i++) {
		int value = i - PAL16_BLUE;
		value += pal16blue;
		value += pal16blueDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16blue), pal16blue + pal16colors - 1);
	}

	// Fill PAL16_YELLOW indices
	for (int i = PAL16_YELLOW; i < PAL16_ORANGE; i++) {
		int value = i - PAL16_YELLOW;
		value += pal16yellow;
		value += pal16yellowDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16yellow), pal16yellow + pal16colors - 1);
	}

	// Fill PAL16_ORANGE indices
	for (int i = PAL16_ORANGE; i < PAL16_RED; i++) {
		int value = i - PAL16_ORANGE;
		value += pal16orange;
		value += pal16orangeDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16orange), pal16orange + pal16colors - 1);
	}

	// Fill PAL16_RED indices
	for (int i = PAL16_RED; i < PAL16_GRAY; i++) {
		int value = i - PAL16_RED;
		value += pal16red;
		value += pal16redDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16red), pal16red + pal16colors - 1);
	}

	// Fill PAL16_GRAY indices
	for (int i = PAL16_GRAY; i < PAL16_GRAY + pal16colors - 1; i++) {
		int value = i - PAL16_GRAY;
		value += pal16gray;
		value += pal16grayDimming;
		customTrn[i] = clamp(value, static_cast<int>(pal16gray), pal16gray + pal16colors - 1);

	}

	for (int i = PAL8_BLUE; i < PAL16_GRAY + pal16colors; i++) {
		if (customTrn[i] == 255) {
			customTrn[i] = 0;
		}
	}

	return customTrn.release();
}

uint8_t *GetInfravisionTRN()
{
	return &LightTables[16 * 256];
}

uint8_t *GetStoneTRN()
{
	return &LightTables[17 * 256];
}

uint8_t *GetPauseTRN()
{
	return &LightTables[18 * 256];
}

std::optional<std::array<uint8_t, 256>> GetClassTRN(Player &player)
{
	std::array<uint8_t, 256> trn;
	const char *path;

	switch (player._pClass) {
	case HeroClass::Warrior:
		path = "plrgfx\\warrior.trn";
		break;
	case HeroClass::Rogue:
		path = "plrgfx\\rogue.trn";
		break;
	case HeroClass::Sorcerer:
		path = "plrgfx\\sorcerer.trn";
		break;
	case HeroClass::Monk:
		path = "plrgfx\\monk.trn";
		break;
	case HeroClass::Bard:
		path = "plrgfx\\bard.trn";
		break;
	case HeroClass::Barbarian:
		path = "plrgfx\\barbarian.trn";
		break;
	}

#ifdef _DEBUG
	if (!debugTRN.empty()) {
		path = debugTRN.c_str();
	}
#endif
	if (LoadOptionalFileInMem(path, &trn[0], 256)) {
		return trn;
	}
	return std::nullopt;
}

} // namespace devilution
