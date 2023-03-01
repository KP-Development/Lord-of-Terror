/**
 * @file trn.hpp
 *
 * Contains most of trn logic
 */
#pragma once

#include "engine/palette.h"
#include "player.h"
#include "utils/stdcompat/optional.hpp"

namespace devilution {

uint8_t *GetCustomTRN(uint8_t pal8blue, int8_t pal8blueDimming, uint8_t pal8red, int8_t pal8redDimming, uint8_t pal8yellow, int8_t pal8yellowDimming, uint8_t pal8orange, int8_t pal8orangeDimming, uint8_t pal16beige, int8_t pal16beigeDimming, uint8_t pal16blue, int8_t pal16blueDimming, uint8_t pal16yellow, int8_t pal16yellowDimming, uint8_t pal16orange, int8_t pal16orangeDimming, uint8_t pal16red, int8_t pal16redDimming, uint8_t pal16gray, int8_t pal16grayDimming);
uint8_t *GetInfravisionTRN();
uint8_t *GetStoneTRN();
uint8_t *GetPauseTRN();
std::optional<std::array<uint8_t, 256>> GetClassTRN(Player &player);

} // namespace devilution
