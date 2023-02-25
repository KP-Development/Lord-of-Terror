/**
 * @file inv.cpp
 *
 * Implementation of player inventory.
 */
#include <utility>

#include <algorithm>
#include <fmt/format.h>
#include <set>

#include "DiabloUI/ui_flags.hpp"
#include "controls/plrctrls.h"
#include "cursor.h"
#include "engine/backbuffer_state.hpp"
#include "engine/clx_sprite.hpp"
#include "engine/load_cel.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/text_render.hpp"
#include "engine/size.hpp"
#include "engine/trn.hpp"
#include "hwcursor.hpp"
#include "inv_iterators.hpp"
#include "levels/town.h"
#include "minitext.h"
#include "options.h"
#include "panels/ui_panels.hpp"
#include "plrmsg.h"
#include "qol/stash.h"
#include "stores.h"
#include "towners.h"
#include "utils/format_int.hpp"
#include "utils/language.h"
#include "utils/sdl_geometry.h"
#include "utils/stdcompat/optional.hpp"
#include "utils/str_cat.hpp"
#include "utils/utf8.hpp"

namespace devilution {

bool invflag;

/**
 * Maps from inventory slot to screen position. The inventory slots are
 * arranged as follows:
 *
 * @code{.unparsed}
 *                          00 01
 *                          02 03   06
 *
 *              07 08       19 20       13 14
 *              09 10       21 22       15 16
 *              11 12       23 24       17 18
 *
 *                 04                   05
 *
 *              25 26 27 28 29 30 31 32 33 34
 *              35 36 37 38 39 40 41 42 43 44
 *              45 46 47 48 49 50 51 52 53 54
 *              55 56 57 58 59 60 61 62 63 64
 *
 * 65 66 67 68 69 70 71 72
 * @endcode
 */
const Point InvRect[] = {
	// clang-format off
	//  X,   Y
	{ 132,  31 }, // helmet
	{ 160,  31 }, // helmet
	{ 132,  59 }, // helmet
	{ 160,  59 }, // helmet
	{  45, 205 }, // left ring
	{ 247, 205 }, // right ring
	{ 204,  59 }, // amulet
	{  17, 104 }, // left hand
	{  46, 104 }, // left hand
	{  17, 132 }, // left hand
	{  46, 132 }, // left hand
	{  17, 160 }, // left hand
	{  46, 160 }, // left hand
	{ 247, 104 }, // right hand
	{ 276, 104 }, // right hand
	{ 247, 132 }, // right hand
	{ 276, 132 }, // right hand
	{ 247, 160 }, // right hand
	{ 276, 160 }, // right hand
	{ 132, 104 }, // chest
	{ 160, 104 }, // chest
	{ 132, 132 }, // chest
	{ 160, 132 }, // chest
	{ 132, 160 }, // chest
	{ 160, 160 }, // chest
	{  17, 250 }, // inv row 1
	{  46, 250 }, // inv row 1
	{  75, 250 }, // inv row 1
	{ 104, 250 }, // inv row 1
	{ 133, 250 }, // inv row 1
	{ 162, 250 }, // inv row 1
	{ 191, 250 }, // inv row 1
	{ 220, 250 }, // inv row 1
	{ 249, 250 }, // inv row 1
	{ 278, 250 }, // inv row 1
	{  17, 279 }, // inv row 2
	{  46, 279 }, // inv row 2
	{  75, 279 }, // inv row 2
	{ 104, 279 }, // inv row 2
	{ 133, 279 }, // inv row 2
	{ 162, 279 }, // inv row 2
	{ 191, 279 }, // inv row 2
	{ 220, 279 }, // inv row 2
	{ 249, 279 }, // inv row 2
	{ 278, 279 }, // inv row 2
	{  17, 308 }, // inv row 3
	{  46, 308 }, // inv row 3
	{  75, 308 }, // inv row 3
	{ 104, 308 }, // inv row 3
	{ 133, 308 }, // inv row 3
	{ 162, 308 }, // inv row 3
	{ 191, 308 }, // inv row 3
	{ 220, 308 }, // inv row 3
	{ 249, 308 }, // inv row 3
	{ 278, 308 }, // inv row 3
	{  17, 337 }, // inv row 4
	{  46, 337 }, // inv row 4
	{  75, 337 }, // inv row 4
	{ 104, 337 }, // inv row 4
	{ 133, 337 }, // inv row 4
	{ 162, 337 }, // inv row 4
	{ 191, 337 }, // inv row 4
	{ 220, 337 }, // inv row 4
	{ 249, 337 }, // inv row 4
	{ 278, 337 }, // inv row 4
	{ 205,  33 }, // belt
	{ 234,  33 }, // belt
	{ 263,  33 }, // belt
	{ 292,  33 }, // belt
	{ 321,  33 }, // belt
	{ 350,  33 }, // belt
	{ 379,  33 }, // belt
	{ 408,  33 }  // belt
	// clang-format on
};

namespace {

OptionalOwnedClxSpriteList pInvCels;

/**
 * @brief Adds an item to a player's InvGrid array
 * @param player The player reference
 * @param invGridIndex Item's position in InvGrid (this should be the item's topleft grid tile)
 * @param invListIndex The item's InvList index (it's expected this already has +1 added to it since InvGrid can't store a 0 index)
 * @param itemSize Size of item
 */
void AddItemToInvGrid(Player &player, int invGridIndex, int invListIndex, Size itemSize)
{
	const int pitch = 10;
	for (int y = 0; y < itemSize.height; y++) {
		int rowGridIndex = invGridIndex + pitch * y;
		for (int x = 0; x < itemSize.width; x++) {
			if (x == 0 && y == itemSize.height - 1)
				player.InvGrid[rowGridIndex + x] = invListIndex;
			else
				player.InvGrid[rowGridIndex + x] = -invListIndex;
		}
	}

	if (&player == MyPlayer) {
		NetSendCmdChInvItem(false, invGridIndex);
	}
}

/**
 * @brief Checks whether the given item can fit in a belt slot (i.e. the item's size in inventory cells is 1x1).
 * @param item The item to be checked.
 * @return 'True' in case the item can fit a belt slot and 'False' otherwise.
 */
bool FitsInBeltSlot(const Item &item)
{
	return GetInventorySize(item) == Size { 1, 1 };
}

/**
 * @brief Checks whether the given item can be equipped. Since this overload doesn't take player information, it only considers
 * general aspects about the item, like if its requirements are met and if the item's target location is valid for the body.
 * @param item The item to check.
 * @return 'True' in case the item could be equipped in a player, and 'False' otherwise.
 */
bool CanEquip(const Item &item)
{
	return item.isEquipment()
	    && item._iStatFlag;
}

/**
 * @brief A specialized version of 'CanEquip(int, Item&, int)' that specifically checks whether the item can be equipped
 * in one/both of the player's hands.
 * @param player The player whose inventory will be checked for compatibility with the item.
 * @param item The item to check.
 * @return 'True' if the player can currently equip the item in either one of his hands (i.e. the required hands are empty and
 * allow the item), and 'False' otherwise.
 */
bool CanWield(Player &player, const Item &item)
{
	if (!CanEquip(item) || IsNoneOf(player.GetItemLocation(item), ILOC_ONEHAND, ILOC_TWOHAND))
		return false;

	Item &leftHandItem = player.InvBody[INVLOC_HAND_LEFT];
	Item &rightHandItem = player.InvBody[INVLOC_HAND_RIGHT];

	if (leftHandItem.isEmpty() && rightHandItem.isEmpty()) {
		return true;
	}

	if (!leftHandItem.isEmpty() && !rightHandItem.isEmpty()) {
		return false;
	}

	Item &occupiedHand = !leftHandItem.isEmpty() ? leftHandItem : rightHandItem;

	// Bard can dual wield swords and maces, so we allow equiping one-handed weapons in her free slot as long as her occupied
	// slot is another one-handed weapon.
	if (player._pClass == HeroClass::Bard) {
		bool occupiedHandIsOneHandedSwordOrMace = player.GetItemLocation(occupiedHand) == ILOC_ONEHAND
		    && IsAnyOf(occupiedHand._itype, ItemType::Sword, ItemType::Mace);

		bool weaponToEquipIsOneHandedSwordOrMace = player.GetItemLocation(item) == ILOC_ONEHAND
		    && IsAnyOf(item._itype, ItemType::Sword, ItemType::Mace);

		if (occupiedHandIsOneHandedSwordOrMace && weaponToEquipIsOneHandedSwordOrMace) {
			return true;
		}
	}

	return player.GetItemLocation(item) == ILOC_ONEHAND
	    && player.GetItemLocation(occupiedHand) == ILOC_ONEHAND
	    && item._iClass != occupiedHand._iClass;
}

/**
 * @brief Checks whether the specified item can be equipped in the desired body location on the player.
 * @param player The player whose inventory will be checked for compatibility with the item.
 * @param item The item to check.
 * @param bodyLocation The location in the inventory to be checked against.
 * @return 'True' if the player can currently equip the item in the specified body location (i.e. the body location is empty and
 * allows the item), and 'False' otherwise.
 */
bool CanEquip(Player &player, const Item &item, inv_body_loc bodyLocation)
{
	if (!CanEquip(item) || player._pmode > PM_WALK_SIDEWAYS || !player.InvBody[bodyLocation].isEmpty()) {
		return false;
	}

	switch (bodyLocation) {
	case INVLOC_AMULET:
		return item._iLoc == ILOC_AMULET;

	case INVLOC_CHEST:
		return item._iLoc == ILOC_ARMOR;

	case INVLOC_HAND_LEFT:
	case INVLOC_HAND_RIGHT:
		return CanWield(player, item);

	case INVLOC_HEAD:
		return item._iLoc == ILOC_HELM;

	case INVLOC_RING_LEFT:
	case INVLOC_RING_RIGHT:
		return item._iLoc == ILOC_RING;

	default:
		return false;
	}
}

/**
 * @brief A specialized version of 'CanEquip(int, Item&, int)' that specifically checks whether the item can be equipped
 * in one/both of the player's hands.
 * @param player The player whose inventory will be checked for compatibility with the item.
 * @param item The item to check.
 * @return 'True' if the player can currently equip the item in either one of his hands (i.e. the required hands are empty and
 * allow the item), and 'False' otherwise.
 */
bool IsValidWield(Player &player, const Item &item)
{
	if (!CanEquip(item) || IsNoneOf(player.GetItemLocation(item), ILOC_ONEHAND, ILOC_TWOHAND))
		return false;

	return true;
}

/**
 * @brief Checks whether the specified item can be equipped in the desired body location on the player.
 * @param player The player whose inventory will be checked for compatibility with the item.
 * @param item The item to check.
 * @param bodyLocation The location in the inventory to be checked against.
 * @return 'True' if the player can currently equip the item in the specified body location and 'False' otherwise.
 */
bool IsValidEquip(Player &player, const Item &item, inv_body_loc bodyLocation)
{
	if (!CanEquip(item) || player._pmode > PM_WALK_SIDEWAYS) {
		return false;
	}

	switch (bodyLocation) {
	case INVLOC_AMULET:
		return item._iLoc == ILOC_AMULET;

	case INVLOC_CHEST:
		return item._iLoc == ILOC_ARMOR;

	case INVLOC_HAND_LEFT:
	case INVLOC_HAND_RIGHT:
		return IsValidWield(player, item);

	case INVLOC_HEAD:
		return item._iLoc == ILOC_HELM;

	case INVLOC_RING_LEFT:
	case INVLOC_RING_RIGHT:
		return item._iLoc == ILOC_RING;

	default:
		return false;
	}
}

void ChangeEquipment(Player &player, inv_body_loc bodyLocation, const Item &item)
{
	player.InvBody[bodyLocation] = item;

	if (&player == MyPlayer) {
		NetSendCmdChItem(false, bodyLocation);
	}
}

bool AutoEquip(Player &player, const Item &item, inv_body_loc bodyLocation, bool persistItem)
{
	if (!CanEquip(player, item, bodyLocation)) {
		return false;
	}

	if (persistItem) {
		ChangeEquipment(player, bodyLocation, item);

		if (*sgOptions.Audio.autoEquipSound && &player == MyPlayer) {
			PlaySFX(ItemInvSnds[ItemCAnimTbl[item._iCurs]]);
		}

		CalcPlrInv(player, true);
	}

	return true;
}

int FindSlotUnderCursor(Point cursorPosition, Size itemSize)
{
	int i = cursorPosition.x;
	int j = cursorPosition.y;

	if (!IsHardwareCursor()) {
		// offset the cursor position to match the hot pixel we'd use for a hardware cursor
		i += itemSize.width * INV_SLOT_HALF_SIZE_PX;
		j += itemSize.height * INV_SLOT_HALF_SIZE_PX;
	}

	for (int r = 0; r < NUM_XY_SLOTS; r++) {
		int xo = GetRightPanel().position.x;
		int yo = GetRightPanel().position.y;
		if (r >= SLOTXY_BELT_FIRST) {
			xo = GetMainPanel().position.x;
			yo = GetMainPanel().position.y;
		}

		if (i >= InvRect[r].x + xo && i <= InvRect[r].x + xo + InventorySlotSizeInPixels.width) {
			if (j >= InvRect[r].y + yo - InventorySlotSizeInPixels.height - 1 && j < InvRect[r].y + yo) {
				return r;
			}
		}
		if (r == SLOTXY_CHEST_LAST) {
			if (itemSize.width % 2 == 0)
				i -= INV_SLOT_HALF_SIZE_PX;
			if (itemSize.height % 2 == 0)
				j -= INV_SLOT_HALF_SIZE_PX;
		}
		if (r == SLOTXY_INV_LAST && itemSize.height % 2 == 0)
			j += INV_SLOT_HALF_SIZE_PX;
	}
	return NUM_XY_SLOTS;
}

void CheckInvPaste(Player &player, Point cursorPosition)
{
	Size itemSize = GetInventorySize(player.HoldItem);

	int slot = FindSlotUnderCursor(cursorPosition, itemSize);
	if (slot == NUM_XY_SLOTS)
		return;

	item_equip_type il = ILOC_UNEQUIPABLE;
	if (slot >= SLOTXY_HEAD_FIRST && slot <= SLOTXY_HEAD_LAST)
		il = ILOC_HELM;
	if (slot >= SLOTXY_RING_LEFT && slot <= SLOTXY_RING_RIGHT)
		il = ILOC_RING;
	if (slot == SLOTXY_AMULET)
		il = ILOC_AMULET;
	if (slot >= SLOTXY_HAND_LEFT_FIRST && slot <= SLOTXY_HAND_RIGHT_LAST)
		il = ILOC_ONEHAND;
	if (slot >= SLOTXY_CHEST_FIRST && slot <= SLOTXY_CHEST_LAST)
		il = ILOC_ARMOR;
	if (slot >= SLOTXY_BELT_FIRST && slot <= SLOTXY_BELT_LAST)
		il = ILOC_BELT;

	item_equip_type desiredIl = player.GetItemLocation(player.HoldItem);
	if (il == ILOC_ONEHAND && desiredIl == ILOC_TWOHAND)
		il = ILOC_TWOHAND;

	int8_t it = 0;
	if (il == ILOC_UNEQUIPABLE) {
		int ii = slot - SLOTXY_INV_FIRST;
		if (player.HoldItem._itype == ItemType::Gold) {
			if (player.InvGrid[ii] != 0) {
				int8_t iv = player.InvGrid[ii];
				if (iv > 0) {
					if (player.InvList[iv - 1]._itype != ItemType::Gold) {
						it = iv;
					}
				} else {
					it = -iv;
				}
			}
		} else {
			int yy = std::max(INV_ROW_SLOT_SIZE * ((ii / INV_ROW_SLOT_SIZE) - ((itemSize.height - 1) / 2)), 0);
			for (int j = 0; j < itemSize.height; j++) {
				if (yy >= InventoryGridCells)
					return;
				int xx = std::max((ii % INV_ROW_SLOT_SIZE) - ((itemSize.width - 1) / 2), 0);
				for (int i = 0; i < itemSize.width; i++) {
					if (xx >= INV_ROW_SLOT_SIZE)
						return;
					if (player.InvGrid[xx + yy] != 0) {
						int8_t iv = abs(player.InvGrid[xx + yy]);
						if (it != 0) {
							if (it != iv) {
								SDL_Log("failed");
								return;
							}
						} else {
							it = iv;
						}
					}
					xx++;
				}
				yy += INV_ROW_SLOT_SIZE;
			}
		}
	} else if (il == ILOC_BELT) {
		if (!CanBePlacedOnBelt(player.HoldItem))
			return;
	} else if (desiredIl != il) {
		return;
	}

	if (IsNoneOf(il, ILOC_UNEQUIPABLE, ILOC_BELT) && !player.CanUseItem(player.HoldItem)) {
		player.Say(HeroSpeech::ICantUseThisYet);
		return;
	}

	if (player._pmode > PM_WALK_SIDEWAYS && IsNoneOf(il, ILOC_UNEQUIPABLE, ILOC_BELT))
		return;

	if (&player == MyPlayer)
		PlaySFX(ItemInvSnds[ItemCAnimTbl[player.HoldItem._iCurs]]);

	switch (il) {
	case ILOC_HELM:
	case ILOC_RING:
	case ILOC_AMULET:
	case ILOC_ARMOR: {
		auto iLocToInvLoc = [&slot](item_equip_type loc) {
			switch (loc) {
			case ILOC_HELM:
				return INVLOC_HEAD;
			case ILOC_RING:
				return (slot == SLOTXY_RING_LEFT ? INVLOC_RING_LEFT : INVLOC_RING_RIGHT);
			case ILOC_AMULET:
				return INVLOC_AMULET;
			case ILOC_ARMOR:
				return INVLOC_CHEST;
			default:
				app_fatal("Unexpected equipment type");
			}
		};
		inv_body_loc slot = iLocToInvLoc(il);
		Item previouslyEquippedItem = player.InvBody[slot];
		ChangeEquipment(player, slot, player.HoldItem.pop());
		if (!previouslyEquippedItem.isEmpty()) {
			player.HoldItem = previouslyEquippedItem;
		}
		break;
	}
	case ILOC_ONEHAND: {
		inv_body_loc selectedHand = slot <= SLOTXY_HAND_LEFT_LAST ? INVLOC_HAND_LEFT : INVLOC_HAND_RIGHT;
		inv_body_loc otherHand = slot <= SLOTXY_HAND_LEFT_LAST ? INVLOC_HAND_RIGHT : INVLOC_HAND_LEFT;

		bool pasteIntoSelectedHand = (player.InvBody[otherHand].isEmpty() || player.InvBody[otherHand]._iClass != player.HoldItem._iClass)
		    || (player._pClass == HeroClass::Bard && player.InvBody[otherHand]._iClass == ICLASS_WEAPON && player.HoldItem._iClass == ICLASS_WEAPON);

		bool dequipTwoHandedWeapon = (!player.InvBody[otherHand].isEmpty() && player.GetItemLocation(player.InvBody[otherHand]) == ILOC_TWOHAND);

		inv_body_loc pasteHand = pasteIntoSelectedHand ? selectedHand : otherHand;
		Item previouslyEquippedItem = dequipTwoHandedWeapon ? player.InvBody[otherHand] : player.InvBody[pasteHand];
		if (dequipTwoHandedWeapon) {
			RemoveEquipment(player, otherHand, false);
		}
		ChangeEquipment(player, pasteHand, player.HoldItem.pop());
		if (!previouslyEquippedItem.isEmpty()) {
			player.HoldItem = previouslyEquippedItem;
		}
		break;
	}
	case ILOC_TWOHAND:
		if (!player.InvBody[INVLOC_HAND_LEFT].isEmpty() && !player.InvBody[INVLOC_HAND_RIGHT].isEmpty()) {
			inv_body_loc locationToUnequip = INVLOC_HAND_LEFT;
			if (player.InvBody[INVLOC_HAND_RIGHT]._itype == ItemType::Shield) {
				locationToUnequip = INVLOC_HAND_RIGHT;
			}
			bool done2h = AutoPlaceItemInInventory(player, player.InvBody[locationToUnequip], true);
			if (!done2h)
				return;

			if (locationToUnequip == INVLOC_HAND_RIGHT) {
				RemoveEquipment(player, INVLOC_HAND_RIGHT, false);
			} else {
				// CMD_CHANGEPLRITEMS will eventually be sent for the left hand
				player.InvBody[INVLOC_HAND_LEFT].clear();
			}
		}

		if (player.InvBody[INVLOC_HAND_RIGHT].isEmpty()) {
			Item previouslyEquippedItem = player.InvBody[INVLOC_HAND_LEFT];
			ChangeEquipment(player, INVLOC_HAND_LEFT, player.HoldItem.pop());
			if (!previouslyEquippedItem.isEmpty()) {
				player.HoldItem = previouslyEquippedItem;
			}
		} else {
			Item previouslyEquippedItem = player.InvBody[INVLOC_HAND_RIGHT];
			RemoveEquipment(player, INVLOC_HAND_RIGHT, false);
			ChangeEquipment(player, INVLOC_HAND_LEFT, player.HoldItem);
			player.HoldItem = previouslyEquippedItem;
		}
		break;
	case ILOC_UNEQUIPABLE:
		if (player.HoldItem._itype == ItemType::Gold && it == 0) {
			int ii = slot - SLOTXY_INV_FIRST;
			if (player.InvGrid[ii] > 0) {
				int invIndex = player.InvGrid[ii] - 1;
				int gt = player.InvList[invIndex]._ivalue;
				int ig = player.HoldItem._ivalue + gt;
				if (ig <= MaxGold) {
					player.InvList[invIndex]._ivalue = ig;
					SetPlrHandGoldCurs(player.InvList[invIndex]);
					player._pGold += player.HoldItem._ivalue;
					player.HoldItem.clear();
				} else {
					ig = MaxGold - gt;
					player._pGold += ig;
					player.HoldItem._ivalue -= ig;
					SetPlrHandGoldCurs(player.HoldItem);
					player.InvList[invIndex]._ivalue = MaxGold;
					player.InvList[invIndex]._iCurs = ICURS_GOLD_LARGE;
				}
			} else {
				int invIndex = player._pNumInv;
				player._pGold += player.HoldItem._ivalue;
				player.InvList[invIndex] = player.HoldItem.pop();
				player._pNumInv++;
				player.InvGrid[ii] = player._pNumInv;
			}
			if (&player == MyPlayer) {
				NetSendCmdChInvItem(false, ii);
			}
		} else {
			if (it == 0) {
				player.InvList[player._pNumInv] = player.HoldItem.pop();
				player._pNumInv++;
				it = player._pNumInv;
			} else {
				int invIndex = it - 1;
				if (player.HoldItem._itype == ItemType::Gold)
					player._pGold += player.HoldItem._ivalue;
				std::swap(player.InvList[invIndex], player.HoldItem);
				if (player.HoldItem._itype == ItemType::Gold)
					player._pGold = CalculateGold(player);
				for (auto &itemIndex : player.InvGrid) {
					if (itemIndex == it)
						itemIndex = 0;
					if (itemIndex == -it)
						itemIndex = 0;
				}
			}
			int ii = slot - SLOTXY_INV_FIRST;

			// Calculate top-left position of item for InvGrid and then add item to InvGrid

			int xx = std::max(ii % INV_ROW_SLOT_SIZE - ((itemSize.width - 1) / 2), 0);
			int yy = std::max(INV_ROW_SLOT_SIZE * (ii / INV_ROW_SLOT_SIZE - ((itemSize.height - 1) / 2)), 0);
			AddItemToInvGrid(player, xx + yy, it, itemSize);
		}
		break;
	case ILOC_BELT: {
		int ii = slot - SLOTXY_BELT_FIRST;
		if (player.SpdList[ii].isEmpty()) {
			player.SpdList[ii] = player.HoldItem.pop();
		} else {
			std::swap(player.SpdList[ii], player.HoldItem);
			if (player.HoldItem._itype == ItemType::Gold)
				player._pGold = CalculateGold(player);
		}
		if (&player == MyPlayer) {
			NetSendCmdChBeltItem(false, ii);
		}
		RedrawComponent(PanelDrawComponent::Belt);
	} break;
	case ILOC_NONE:
	case ILOC_INVALID:
		break;
	}
	CalcPlrInv(player, true);
	if (&player == MyPlayer) {
		if (player.HoldItem.isEmpty() && !IsHardwareCursor())
			SetCursorPos(MousePosition + Displacement { itemSize * INV_SLOT_HALF_SIZE_PX });
		NewCursor(player.HoldItem);
	}
}

void CheckInvCut(Player &player, Point cursorPosition, bool automaticMove, bool dropItem)
{
	if (player._pmode > PM_WALK_SIDEWAYS) {
		return;
	}

	if (dropGoldFlag) {
		CloseGoldDrop();
		dropGoldValue = 0;
	}

	uint32_t r = 0;
	for (; r < NUM_XY_SLOTS; r++) {
		int xo = GetRightPanel().position.x;
		int yo = GetRightPanel().position.y;
		if (r >= SLOTXY_BELT_FIRST) {
			xo = GetMainPanel().position.x;
			yo = GetMainPanel().position.y;
		}

		// check which inventory rectangle the mouse is in, if any
		if (cursorPosition.x >= InvRect[r].x + xo
		    && cursorPosition.x < InvRect[r].x + xo + (InventorySlotSizeInPixels.width + 1)
		    && cursorPosition.y >= InvRect[r].y + yo - (InventorySlotSizeInPixels.height + 1)
		    && cursorPosition.y < InvRect[r].y + yo) {
			break;
		}
	}

	if (r == NUM_XY_SLOTS) {
		// not on an inventory slot rectangle
		return;
	}

	Item &holdItem = player.HoldItem;
	holdItem.clear();

	bool automaticallyMoved = false;
	bool automaticallyEquipped = false;
	bool automaticallyUnequip = false;

	Item &headItem = player.InvBody[INVLOC_HEAD];
	if (r >= SLOTXY_HEAD_FIRST && r <= SLOTXY_HEAD_LAST && !headItem.isEmpty()) {
		holdItem = headItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_HEAD, false);
		}
	}

	Item &leftRingItem = player.InvBody[INVLOC_RING_LEFT];
	if (r == SLOTXY_RING_LEFT && !leftRingItem.isEmpty()) {
		holdItem = leftRingItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_RING_LEFT, false);
		}
	}

	Item &rightRingItem = player.InvBody[INVLOC_RING_RIGHT];
	if (r == SLOTXY_RING_RIGHT && !rightRingItem.isEmpty()) {
		holdItem = rightRingItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_RING_RIGHT, false);
		}
	}

	Item &amuletItem = player.InvBody[INVLOC_AMULET];
	if (r == SLOTXY_AMULET && !amuletItem.isEmpty()) {
		holdItem = amuletItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_AMULET, false);
		}
	}

	Item &leftHandItem = player.InvBody[INVLOC_HAND_LEFT];
	if (r >= SLOTXY_HAND_LEFT_FIRST && r <= SLOTXY_HAND_LEFT_LAST && !leftHandItem.isEmpty()) {
		holdItem = leftHandItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_HAND_LEFT, false);
		}
	}

	Item &rightHandItem = player.InvBody[INVLOC_HAND_RIGHT];
	if (r >= SLOTXY_HAND_RIGHT_FIRST && r <= SLOTXY_HAND_RIGHT_LAST && !rightHandItem.isEmpty()) {
		holdItem = rightHandItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_HAND_RIGHT, false);
		}
	}

	Item &chestItem = player.InvBody[INVLOC_CHEST];
	if (r >= SLOTXY_CHEST_FIRST && r <= SLOTXY_CHEST_LAST && !chestItem.isEmpty()) {
		holdItem = chestItem;
		if (automaticMove) {
			automaticallyUnequip = true;
			automaticallyMoved = automaticallyEquipped = AutoPlaceItemInInventory(player, holdItem, true);
		}

		if (!automaticMove || automaticallyMoved) {
			RemoveEquipment(player, INVLOC_CHEST, false);
		}
	}

	if (r >= SLOTXY_INV_FIRST && r <= SLOTXY_INV_LAST) {
		int ig = r - SLOTXY_INV_FIRST;
		int8_t ii = player.InvGrid[ig];
		if (ii != 0) {
			int iv = (ii < 0) ? -ii : ii;

			holdItem = player.InvList[iv - 1];
			if (automaticMove) {
				if (CanBePlacedOnBelt(holdItem)) {
					automaticallyMoved = AutoPlaceItemInBelt(player, holdItem, true);
				} else if (CanEquip(holdItem)) {
					/*
					 * Move the respective InvBodyItem to inventory before moving the item from inventory
					 * to InvBody with AutoEquip. AutoEquip requires the InvBody slot to be empty.
					 * First identify the correct InvBody slot and store it in invloc.
					 */
					automaticallyUnequip = true; // Switch to say "I have no room when inventory is too full"
					int invloc = NUM_INVLOC;
					switch (player.GetItemLocation(holdItem)) {
					case ILOC_ARMOR:
						invloc = INVLOC_CHEST;
						break;
					case ILOC_HELM:
						invloc = INVLOC_HEAD;
						break;
					case ILOC_AMULET:
						invloc = INVLOC_AMULET;
						break;
					case ILOC_ONEHAND:
						// User is attempting to move a weapon (left hand)
						if (player.InvList[iv - 1]._iClass == player.InvBody[INVLOC_HAND_LEFT]._iClass
						    && player.GetItemLocation(player.InvList[iv - 1]) == player.GetItemLocation(player.InvBody[INVLOC_HAND_LEFT])) {
							invloc = INVLOC_HAND_LEFT;
						}
						// User is attempting to move a shield (right hand)
						if (player.InvList[iv - 1]._iClass == player.InvBody[INVLOC_HAND_RIGHT]._iClass
						    && player.GetItemLocation(player.InvList[iv - 1]) == player.GetItemLocation(player.InvBody[INVLOC_HAND_RIGHT])) {
							invloc = INVLOC_HAND_RIGHT;
						}
						// A two-hand item can always be replaced with a one-hand item
						if (player.GetItemLocation(player.InvBody[INVLOC_HAND_LEFT]) == ILOC_TWOHAND) {
							invloc = INVLOC_HAND_LEFT;
						}
						break;
					case ILOC_TWOHAND:
						// Moving a two-hand item from inventory to InvBody requires emptying both hands
						if (!player.InvBody[INVLOC_HAND_RIGHT].isEmpty()) {
							holdItem = player.InvBody[INVLOC_HAND_RIGHT];
							if (!AutoPlaceItemInInventory(player, holdItem, true)) {
								// No space to  move right hand item to inventory, abort.
								break;
							}
							holdItem = player.InvBody[INVLOC_HAND_LEFT];
							if (!AutoPlaceItemInInventory(player, holdItem, false)) {
								// No space for left item. Move back right item to right hand and abort.
								player.InvBody[INVLOC_HAND_RIGHT] = player.InvList[player._pNumInv - 1];
								player.RemoveInvItem(player._pNumInv - 1, false);
								break;
							}
							RemoveEquipment(player, INVLOC_HAND_RIGHT, false);
							invloc = INVLOC_HAND_LEFT;
						} else {
							invloc = INVLOC_HAND_LEFT;
						}
						break;
					default:
						automaticallyUnequip = false; // Switch to say "I can't do that"
						break;
					}
					// Empty the identified InvBody slot (invloc) and hand over to AutoEquip
					if (invloc != NUM_INVLOC) {
						holdItem = player.InvBody[invloc];
						if (player.InvBody[invloc]._itype != ItemType::None) {
							if (AutoPlaceItemInInventory(player, holdItem, true)) {
								player.InvBody[invloc].clear();
							}
						}
					}
					holdItem = player.InvList[iv - 1];
					automaticallyMoved = automaticallyEquipped = AutoEquip(player, holdItem);
				}
			}

			if (!automaticMove || automaticallyMoved) {
				player.RemoveInvItem(iv - 1, false);
			}
		}
	}

	if (r >= SLOTXY_BELT_FIRST) {
		Item &beltItem = player.SpdList[r - SLOTXY_BELT_FIRST];
		if (!beltItem.isEmpty()) {
			holdItem = beltItem;
			if (automaticMove) {
				automaticallyMoved = AutoPlaceItemInInventory(player, holdItem, true);
			}

			if (!automaticMove || automaticallyMoved) {
				player.RemoveSpdBarItem(r - SLOTXY_BELT_FIRST);
			}
		}
	}

	if (!holdItem.isEmpty()) {
		if (holdItem._itype == ItemType::Gold) {
			player._pGold = CalculateGold(player);
		}

		CalcPlrInv(player, true);
		holdItem._iStatFlag = player.CanUseItem(holdItem);

		if (&player == MyPlayer) {
			if (automaticallyEquipped) {
				PlaySFX(ItemInvSnds[ItemCAnimTbl[holdItem._iCurs]]);
			} else if (!automaticMove || automaticallyMoved) {
				PlaySFX(IS_IGRAB);
			}

			if (automaticMove) {
				if (!automaticallyMoved) {
					if (CanBePlacedOnBelt(holdItem) || automaticallyUnequip) {
						player.SaySpecific(HeroSpeech::IHaveNoRoom);
					} else {
						player.SaySpecific(HeroSpeech::ICantDoThat);
					}
				}

				holdItem.clear();
			} else {
				NewCursor(holdItem);
				if (!IsHardwareCursor() && !dropItem) {
					// For a hardware cursor, we set the "hot point" to the center of the item instead.
					Size cursSize = GetInvItemSize(holdItem._iCurs + CURSOR_FIRSTITEM);
					SetCursorPos(cursorPosition - Displacement(cursSize / 2));
				}
			}
		}
	}

	if (dropItem && !holdItem.isEmpty()) {
		TryDropItem();
	}
}

void TryCombineNaKrulNotes(Player &player, Item &noteItem)
{
	int idx = noteItem.IDidx;
	_item_indexes notes[] = { IDI_NOTE1, IDI_NOTE2, IDI_NOTE3 };

	if (IsNoneOf(idx, IDI_NOTE1, IDI_NOTE2, IDI_NOTE3)) {
		return;
	}

	for (_item_indexes note : notes) {
		if (idx != note && !HasInventoryItemWithId(player, note)) {
			return; // the player doesn't have all notes
		}
	}

	MyPlayer->Say(HeroSpeech::JustWhatIWasLookingFor, 10);

	for (_item_indexes note : notes) {
		if (idx != note) {
			RemoveInventoryItemById(player, note);
		}
	}

	Point position = noteItem.position; // copy the position to restore it after re-initialising the item
	noteItem = {};
	GetItemAttrs(noteItem, IDI_FULLNOTE, 16);
	SetupItem(noteItem);
	noteItem.position = position; // this ensures CleanupItem removes the entry in the dropped items lookup table
}

void CheckQuestItem(Player &player, Item &questItem)
{
	Player &myPlayer = *MyPlayer;

	if (questItem.IDidx == IDI_OPTAMULET && Quests[Q_BLIND]._qactive == QUEST_ACTIVE) {
		Quests[Q_BLIND]._qactive = QUEST_DONE;
		NetSendCmdQuest(true, Quests[Q_BLIND]);
	}

	if (questItem.IDidx == IDI_MUSHROOM && Quests[Q_MUSHROOM]._qactive == QUEST_ACTIVE && Quests[Q_MUSHROOM]._qvar1 == QS_MUSHSPAWNED) {
		player.Say(HeroSpeech::NowThatsOneBigMushroom, 10); // BUGFIX: Voice for this quest might be wrong in MP
		Quests[Q_MUSHROOM]._qvar1 = QS_MUSHPICKED;
		NetSendCmdQuest(true, Quests[Q_MUSHROOM]);
	}

	if (questItem.IDidx == IDI_ANVIL && Quests[Q_ANVIL]._qactive != QUEST_NOTAVAIL) {
		if (Quests[Q_ANVIL]._qactive == QUEST_INIT) {
			Quests[Q_ANVIL]._qactive = QUEST_ACTIVE;
			NetSendCmdQuest(true, Quests[Q_ANVIL]);
		}
		if (Quests[Q_ANVIL]._qlog) {
			myPlayer.Say(HeroSpeech::INeedToGetThisToGriswold, 10);
		}
	}

	if (questItem.IDidx == IDI_GLDNELIX && Quests[Q_VEIL]._qactive != QUEST_NOTAVAIL) {
		myPlayer.Say(HeroSpeech::INeedToGetThisToLachdanan, 30);
	}

	if (questItem.IDidx == IDI_ROCK && Quests[Q_ROCK]._qactive != QUEST_NOTAVAIL) {
		if (Quests[Q_ROCK]._qactive == QUEST_INIT) {
			Quests[Q_ROCK]._qactive = QUEST_ACTIVE;
			NetSendCmdQuest(true, Quests[Q_ROCK]);
		}
		if (Quests[Q_ROCK]._qlog) {
			myPlayer.Say(HeroSpeech::ThisMustBeWhatGriswoldWanted, 10);
		}
	}

	if (questItem.IDidx == IDI_ARMOFVAL && Quests[Q_BLOOD]._qactive == QUEST_ACTIVE) {
		Quests[Q_BLOOD]._qactive = QUEST_DONE;
		NetSendCmdQuest(true, Quests[Q_BLOOD]);
		myPlayer.Say(HeroSpeech::MayTheSpiritOfArkaineProtectMe, 20);
	}

	if (questItem.IDidx == IDI_MAPOFDOOM) {
		Quests[Q_GRAVE]._qlog = false;
		Quests[Q_GRAVE]._qactive = QUEST_ACTIVE;
		if (Quests[Q_GRAVE]._qvar1 != 1) {
			MyPlayer->Say(HeroSpeech::UhHuh, 10);
			Quests[Q_GRAVE]._qvar1 = 1;
		}
	}

	TryCombineNaKrulNotes(player, questItem);
}

void CleanupItems(int ii)
{
	auto &item = Items[ii];
	dItem[item.position.x][item.position.y] = 0;

	if (currlevel == 21 && item.position == CornerStone.position) {
		CornerStone.item.clear();
		CornerStone.item._iSelFlag = 0;
		CornerStone.item.position = { 0, 0 };
		CornerStone.item._iAnimFlag = false;
		CornerStone.item._iIdentified = false;
		CornerStone.item._iPostDraw = false;
	}

	int i = 0;
	while (i < ActiveItemCount) {
		if (ActiveItems[i] == ii) {
			DeleteItem(i);
			i = 0;
			continue;
		}

		i++;
	}
}

bool CanUseStaff(Item &staff, SpellID spell)
{
	return !staff.isEmpty()
	    && IsAnyOf(staff._iMiscId, IMISC_STAFF, IMISC_UNIQUE)
	    && staff._iSpell == spell
	    && staff._iCharges > 0;
}

void StartGoldDrop()
{
	CloseGoldWithdraw();

	initialDropGoldIndex = pcursinvitem;

	Player &myPlayer = *MyPlayer;

	if (pcursinvitem <= INVITEM_INV_LAST)
		initialDropGoldValue = myPlayer.InvList[pcursinvitem - INVITEM_INV_FIRST]._ivalue;
	else
		initialDropGoldValue = myPlayer.SpdList[pcursinvitem - INVITEM_BELT_FIRST]._ivalue;

	if (talkflag)
		control_reset_talk();

	Point start = GetPanelPosition(UiPanels::Inventory, { 67, 128 });
	SDL_Rect rect = MakeSdlRect(start.x, start.y, 180, 20);
	SDL_SetTextInputRect(&rect);

	dropGoldFlag = true;
	dropGoldValue = 0;
	SDL_StartTextInput();
}

int CreateGoldItemInInventorySlot(Player &player, int slotIndex, int value)
{
	if (player.InvGrid[slotIndex] != 0) {
		return value;
	}

	Item &goldItem = player.InvList[player._pNumInv];
	MakeGoldStack(goldItem, std::min(value, MaxGold));
	player._pNumInv++;
	player.InvGrid[slotIndex] = player._pNumInv;
	if (&player == MyPlayer) {
		NetSendCmdChInvItem(false, slotIndex);
	}

	value -= goldItem._ivalue;

	return value;
}

} // namespace

void InvDrawHLight(const Surface &out, Point targetPosition, Size size)
{
	SDL_Rect srcRect = MakeSdlRect(0, 0, size.width, size.height);
	out.Clip(&srcRect, &targetPosition);
	if (size.width <= 0 || size.height <= 0)
		return;

	std::uint8_t *dst = &out[targetPosition];
	const auto dstPitch = out.pitch();

	for (int hgt = size.height; hgt != 0; hgt--, dst -= dstPitch + size.width) {
		for (int wdt = size.width; wdt != 0; wdt--) {
			std::uint8_t pix = *dst;
			pix += 1;
			*dst++ = pix;
		}
	}
}

void InvDrawSlotBack(const Surface &out, Point targetPosition, Size size, Item &item, bool hLight)
{
	SDL_Rect srcRect = MakeSdlRect(0, 0, size.width, size.height);
	out.Clip(&srcRect, &targetPosition);
	if (size.width <= 0 || size.height <= 0)
		return;

	std::uint8_t *dst = &out[targetPosition];
	const auto dstPitch = out.pitch();
	const bool usable = item._iStatFlag;
	int adjustment = -1;
	if (hLight)
		adjustment += 1;

	for (int hgt = size.height; hgt != 0; hgt--, dst -= dstPitch + size.width) {
		for (int wdt = size.width; wdt != 0; wdt--) {
			std::uint8_t pix = *dst;
			if (pix >= PAL16_GRAY) {
				if (usable) {
					switch (item._iMagical) {
					case ITEM_QUALITY_MAGIC:
						pix -= PAL16_GRAY - PAL16_BLUE + adjustment;
						break;
					case ITEM_QUALITY_UNIQUE:
						pix -= PAL16_GRAY - PAL16_YELLOW + adjustment;
						break;
					default:
						pix -= PAL16_GRAY - PAL16_BEIGE + adjustment;
						break;
					}
				} else {
					pix -= PAL16_GRAY - PAL16_RED + 1 + adjustment;
				}
			}
			*dst++ = pix;
		}
	}
}

void InvDrawSwapSlotBack(const Surface &out, Point targetPosition, Size size, bool canPlace /*= true*/)
{
	SDL_Rect srcRect = MakeSdlRect(0, 0, size.width, size.height);
	out.Clip(&srcRect, &targetPosition);
	if (size.width <= 0 || size.height <= 0)
		return;

	std::uint8_t *dst = &out[targetPosition];
	const auto dstPitch = out.pitch();

	for (int hgt = size.height; hgt != 0; hgt--, dst -= dstPitch + size.width) {
		for (int wdt = size.width; wdt != 0; wdt--) {
			std::uint8_t pix = *dst;
			if (pix >= PAL16_GRAY) {
				if (canPlace)
					pix -= PAL16_GRAY - PAL16_ORANGE;
				else
					pix -= PAL16_GRAY - PAL16_RED + 2;
			}
			*dst++ = pix;
		}
	}
}

bool CanBePlacedOnBelt(const Item &item)
{
	return FitsInBeltSlot(item)
	    && item._itype != ItemType::Gold
	    && MyPlayer->CanUseItem(item)
	    && item.isUsable();
}

void FreeInvGFX()
{
	pInvCels = std::nullopt;
}

void InitInv()
{
	switch (MyPlayer->_pClass) {
	case HeroClass::Warrior:
	case HeroClass::Barbarian:
		pInvCels = LoadCel("data\\inv\\inv", static_cast<uint16_t>(SidePanelSize.width));
		break;
	case HeroClass::Rogue:
	case HeroClass::Bard:
		pInvCels = LoadCel("data\\inv\\inv_rog", static_cast<uint16_t>(SidePanelSize.width));
		break;
	case HeroClass::Sorcerer:
		pInvCels = LoadCel("data\\inv\\inv_sor", static_cast<uint16_t>(SidePanelSize.width));
		break;
	case HeroClass::Monk:
		pInvCels = LoadCel(!gbIsSpawn ? "data\\inv\\inv_sor" : "data\\inv\\inv", static_cast<uint16_t>(SidePanelSize.width));
		break;
	}
}

void DrawInv(const Surface &out)
{
	ClxDraw(out, GetPanelPosition(UiPanels::Inventory, { 0, 351 }), (*pInvCels)[0]);

	Player &myPlayer = *MyPlayer;

	Size heldItemSize = GetInventorySize(myPlayer.HoldItem);
	Size heldItemSizeInPixels { heldItemSize.width * InventorySlotSizeInPixels.width, heldItemSize.height * InventorySlotSizeInPixels.height };
	Item &heldItem = myPlayer.HoldItem;

	int slot = FindSlotUnderCursor(MousePosition, heldItemSize);
	int ii = slot - SLOTXY_INV_FIRST;
	bool hLight = false;
	Item &cursItem = GetInventoryItem(myPlayer, pcursinvitem);

	bool highlight = false;
	bool canPlace = true;

	Rectangle mouseRect {
		MousePosition + Displacement { (heldItemSizeInPixels.width / 2) - ((heldItemSize.width - 1) * InventorySlotSizeInPixels.width / 2) - (heldItemSize.width - 1), (heldItemSizeInPixels.height / 2) - ((heldItemSize.height - 1) * InventorySlotSizeInPixels.height / 2) - (heldItemSize.height - 1) - 1 },
		{ ((heldItemSize.width - 1) * InventorySlotSizeInPixels.width) + (heldItemSize.width) + 1,
		    ((heldItemSize.height - 1) * InventorySlotSizeInPixels.height) + (heldItemSize.height) + 1 }
	};
	if (heldItemSize.width == 1)
		mouseRect.position.x -= 1;
	if (heldItemSize.height == 1)
		mouseRect.position.y -= 1;

	Size slotSize[] = {
		{ 2, 2 }, // head
		{ 1, 1 }, // left ring
		{ 1, 1 }, // right ring
		{ 1, 1 }, // amulet
		{ 2, 3 }, // left hand
		{ 2, 3 }, // right hand
		{ 2, 3 }, // chest
	};

	Point slotPos[] = {
		{ 133, 59 },  // head
		{ 48, 205 },  // left ring
		{ 249, 205 }, // right ring
		{ 205, 60 },  // amulet
		{ 17, 160 },  // left hand
		{ 248, 160 }, // right hand
		{ 133, 160 }, // chest
	};

	// Draw equipment slots
	for (int slot = INVLOC_HEAD; slot < NUM_INVLOC; slot++) {
		Item &selectedItem = myPlayer.InvBody[slot];
		const Size slotSizeInPixels { slotSize[slot].width * InventorySlotSizeInPixels.width, slotSize[slot].height * InventorySlotSizeInPixels.height };

		// We need to set highlight and canPlace to false and true respectively to reset them from previous loops
		highlight = false;
		canPlace = true;

		// If the item can't be equipped, or if we're hoving over a slot the item cannot be equipped to
		if (!myPlayer.CanUseItem(heldItem) || !IsValidEquip(myPlayer, heldItem, static_cast<inv_body_loc>(slot)))
			canPlace = false;

		// If we are hovering over an equipped item, make the sprite and slot back brighter
		if (&cursItem == &selectedItem)
			highlight = true;

		// If the iterated slot has an equipped item
		if (!selectedItem.isEmpty()) {
			Point selectedItemPos { slotPos[slot].x, slotPos[slot].y };

			// If holding an item, and hovering over an equipped item, draw swap slot back
			if (!heldItem.isEmpty() && &selectedItem == &cursItem) {
				InvDrawSwapSlotBack(
				    out,
				    GetPanelPosition(UiPanels::Inventory, { selectedItemPos.x, selectedItemPos.y }),
				    slotSizeInPixels,
				    canPlace);
			} else {
				// Draw slot back behind item
				InvDrawSlotBack(
				    out,
				    GetPanelPosition(UiPanels::Inventory, { selectedItemPos.x, selectedItemPos.y }),
				    slotSizeInPixels,
				    selectedItem,
				    highlight);
			}

			const int cursId = selectedItem._iCurs + CURSOR_FIRSTITEM;

			auto selectedItemSize = GetInvItemSize(cursId);

			// calc item offsets for items that do not match the slot size
			selectedItemPos.x += (slotSizeInPixels.width - selectedItemSize.width) / 2;
			selectedItemPos.y -= (slotSizeInPixels.height - selectedItemSize.height) / 2;

			const ClxSprite sprite = GetInvItemSprite(cursId);
			const Point position = GetPanelPosition(UiPanels::Inventory, { selectedItemPos.x, selectedItemPos.y });

			DrawItem(selectedItem, out, position, sprite, highlight);

			// If a two-handed item is equipped, draw the sprite and slot back on the right weapon slot
			if (slot == INVLOC_HAND_LEFT) {
				if (myPlayer.GetItemLocation(selectedItem) == ILOC_TWOHAND) {
					if (!heldItem.isEmpty() && &selectedItem == &cursItem) {
						InvDrawSwapSlotBack(
						    out,
						    GetPanelPosition(UiPanels::Inventory, slotPos[INVLOC_HAND_RIGHT]),
						    slotSizeInPixels,
						    canPlace);
					} else {
						InvDrawSlotBack(
						    out,
						    GetPanelPosition(UiPanels::Inventory, slotPos[INVLOC_HAND_RIGHT]),
						    slotSizeInPixels,
						    selectedItem,
						    highlight);
					}
					LightTableIndex = 0;

					const int dstX = GetRightPanel().position.x + slotPos[INVLOC_HAND_RIGHT].x + (selectedItemSize.width == InventorySlotSizeInPixels.width ? INV_SLOT_HALF_SIZE_PX : 0) - 1;
					const int dstY = GetRightPanel().position.y + slotPos[INVLOC_HAND_RIGHT].y;
					ClxDrawLightBlended(out, { dstX, dstY }, sprite, highlight);
				}
			}
		// If there is no item in the equipment slot we are iterating, and currently are holding an item
		} else if (!heldItem.isEmpty()) {
			// Get a rectangle area of the current iterated slot
			Rectangle slotRect {
				GetPanelPosition(UiPanels::Inventory, slotPos[slot] + Displacement { 0, -slotSizeInPixels.height }),
				{ slotSizeInPixels.width,
				    slotSizeInPixels.height }
			};
			// Draw swap slot back in the iterated slot
			if (slotRect.contains(MousePosition + Displacement { heldItemSizeInPixels.width / 2, heldItemSizeInPixels.height / 2 })) {
				InvDrawSwapSlotBack(
				    out,
				    GetPanelPosition(UiPanels::Inventory, slotPos[slot]),
				    slotSizeInPixels,
				    canPlace);
			}
		}
	}

	// Draw inventory slot backs
	for (int i = 0; i < InventoryGridCells; i++) {
		Item &selectedItem = myPlayer.InvList[abs(myPlayer.InvGrid[i]) - 1];
		const int8_t &selectedItemSlot = myPlayer.InvGrid[i];
		const Point &selectedItemPos = InvRect[i + SLOTXY_INV_FIRST];
		const Point position = GetPanelPosition(UiPanels::Inventory, selectedItemPos) + Displacement { 0, -1 };

		// We need to set highlight and canPlace to false and true respectively to reset them from previous loops
		highlight = false;
		canPlace = true;

		// Get a rectangle area of the current iterated slot
		Rectangle slotRect {
			position + Displacement { 0, -1 - InventorySlotSizeInPixels.height },
			InventorySlotSizeInPixels
		};

		// If holding an item and hovering over an item, draw swap slot back instead of slot back OR if holding an item and the cursor graphic is over a slot, color that slot with swap slot back
		if ((!heldItem.isEmpty() && selectedItemSlot != 0 && &selectedItem == &cursItem) || (!heldItem.isEmpty() && mouseRect.intersects(slotRect))) {
			// If the cursor rectangle overlaps any slot rectangles
				std::set<int8_t> overlappedSlots;
				// Iterate over all the slots
				for (int ii = 0; ii < InventoryGridCells; ii++) {
					const int8_t &iteratedItemSlot = myPlayer.InvGrid[ii];
					const Point &iteratedItemPos = InvRect[ii + SLOTXY_INV_FIRST];
					const Point &iteratedPosition = GetPanelPosition(UiPanels::Inventory, iteratedItemPos) + Displacement { 0, -1 };
					Rectangle iteratedSlotRect {
						iteratedPosition + Displacement { 0, -1 - InventorySlotSizeInPixels.height },
						InventorySlotSizeInPixels
					};
					// If the slot overlaps the mouse rectangle and is not empty, add it to the set
					if (mouseRect.intersects(iteratedSlotRect) && iteratedItemSlot != 0) {
						overlappedSlots.insert(abs(iteratedItemSlot));
					}
				}
				// If the number of overlapped slots is greater than 1, we can't place the item
				if (overlappedSlots.size() > 1) {
					canPlace = false;
				}	
			InvDrawSwapSlotBack(
			    out,
			    position,
			    InventorySlotSizeInPixels,
				canPlace);
		// Otherwise, color any slots that contain items
		} else if (selectedItemSlot != 0) {
			bool highlight = false;
			if (&cursItem == &selectedItem)
				highlight = true;
			InvDrawSlotBack(
			    out,
			    position,
			    InventorySlotSizeInPixels,
			    selectedItem,
				highlight);
		}
	}

	// Draw inventory sprites
	for (int j = 0; j < InventoryGridCells; j++) {
		if (myPlayer.InvGrid[j] > 0) { // first slot of an item
			int ii = myPlayer.InvGrid[j] - 1;
			auto &selectedItem = myPlayer.InvList[ii];
			int cursId = selectedItem._iCurs + CURSOR_FIRSTITEM;

			const ClxSprite sprite = GetInvItemSprite(cursId);
			const Point position = GetPanelPosition(UiPanels::Inventory, InvRect[j + SLOTXY_INV_FIRST]) + Displacement { 0, -1 };

			bool highlight = false;
			if (pcursinvitem == ii + INVITEM_INV_FIRST)
				highlight = true;
			DrawItem(selectedItem, out, position, sprite, highlight);
		}
	}
}

void DrawInvBelt(const Surface &out)
{
	if (talkflag) {
		return;
	}

	Player &myPlayer = *MyPlayer;
	const Point mainPanelPosition = GetMainPanel().position;

	DrawPanelBox(out, { 205, 21, 232, 28 }, mainPanelPosition + Displacement { 205, 5 });

	for (int i = 0; i < MaxBeltItems; i++) {
		Item &selectedItem = myPlayer.SpdList[i];
		const Point &selectedItemPos = InvRect[i + SLOTXY_BELT_FIRST];
		if (selectedItem.isEmpty()) {
			continue;
		}

		const Point position { selectedItemPos.x + mainPanelPosition.x, selectedItemPos.y + mainPanelPosition.y - 1 };
		InvDrawSlotBack(out, position, InventorySlotSizeInPixels, selectedItem);
		const int cursId = selectedItem._iCurs + CURSOR_FIRSTITEM;

		const ClxSprite sprite = GetInvItemSprite(cursId);

		if (pcursinvitem == i + INVITEM_BELT_FIRST) {
			if (ControlMode == ControlTypes::KeyboardAndMouse || invflag) {
				ClxDrawOutline(out, GetOutlineColor(selectedItem, true), position, sprite);
			}
		}

		DrawItem(selectedItem, out, position, sprite);

		if (selectedItem.isUsable()
		    && selectedItem._itype != ItemType::Gold) {
			DrawString(out, StrCat(i + 1), { position - Displacement { 0, 12 }, InventorySlotSizeInPixels }, UiFlags::ColorWhite | UiFlags::AlignRight);
		}
	}
}

void RemoveEquipment(Player &player, inv_body_loc bodyLocation, bool hiPri)
{
	if (&player == MyPlayer) {
		NetSendCmdDelItem(hiPri, bodyLocation);
	}

	player.InvBody[bodyLocation].clear();
}

bool AutoPlaceItemInBelt(Player &player, const Item &item, bool persistItem)
{
	if (!CanBePlacedOnBelt(item)) {
		return false;
	}

	for (auto &beltItem : player.SpdList) {
		if (beltItem.isEmpty()) {
			if (persistItem) {
				beltItem = item;
				player.CalcScrolls();
				RedrawComponent(PanelDrawComponent::Belt);
				if (&player == MyPlayer) {
					size_t beltIndex = std::distance<const Item *>(&player.SpdList[0], &beltItem);
					NetSendCmdChBeltItem(false, beltIndex);
				}
			}

			return true;
		}
	}

	return false;
}

bool AutoEquip(Player &player, const Item &item, bool persistItem)
{
	if (!CanEquip(item)) {
		return false;
	}

	for (int bodyLocation = INVLOC_HEAD; bodyLocation < NUM_INVLOC; bodyLocation++) {
		if (AutoEquip(player, item, (inv_body_loc)bodyLocation, persistItem)) {
			return true;
		}
	}

	return false;
}

bool AutoEquipEnabled(const Player &player, const Item &item)
{
	if (item.isWeapon()) {
		// Monk can use unarmed attack as an encouraged option, thus we do not automatically equip weapons on him so as to not
		// annoy players who prefer that playstyle.
		return player._pClass != HeroClass::Monk && *sgOptions.Gameplay.autoEquipWeapons;
	}

	if (item.isArmor()) {
		return *sgOptions.Gameplay.autoEquipArmor;
	}

	if (item.isHelm()) {
		return *sgOptions.Gameplay.autoEquipHelms;
	}

	if (item.isShield()) {
		return *sgOptions.Gameplay.autoEquipShields;
	}

	if (item.isJewelry()) {
		return *sgOptions.Gameplay.autoEquipJewelry;
	}

	return true;
}

bool AutoPlaceItemInInventory(Player &player, const Item &item, bool persistItem)
{
	Size itemSize = GetInventorySize(item);

	if (itemSize.height == 1) {
		for (int i = 30; i <= 39; i++) {
			if (AutoPlaceItemInInventorySlot(player, i, item, persistItem))
				return true;
		}
		for (int x = 9; x >= 0; x--) {
			for (int y = 2; y >= 0; y--) {
				if (AutoPlaceItemInInventorySlot(player, 10 * y + x, item, persistItem))
					return true;
			}
		}
		return false;
	}

	if (itemSize.height == 2) {
		for (int x = 10 - itemSize.width; x >= 0; x -= itemSize.width) {
			for (int y = 0; y < 3; y++) {
				if (AutoPlaceItemInInventorySlot(player, 10 * y + x, item, persistItem))
					return true;
			}
		}
		if (itemSize.width == 2) {
			for (int x = 7; x >= 0; x -= 2) {
				for (int y = 0; y < 3; y++) {
					if (AutoPlaceItemInInventorySlot(player, 10 * y + x, item, persistItem))
						return true;
				}
			}
		}
		return false;
	}

	if (itemSize == Size { 1, 3 }) {
		for (int i = 0; i < 20; i++) {
			if (AutoPlaceItemInInventorySlot(player, i, item, persistItem))
				return true;
		}
		return false;
	}

	if (itemSize == Size { 2, 3 }) {
		for (int i = 0; i < 9; i++) {
			if (AutoPlaceItemInInventorySlot(player, i, item, persistItem))
				return true;
		}

		for (int i = 10; i < 19; i++) {
			if (AutoPlaceItemInInventorySlot(player, i, item, persistItem))
				return true;
		}
		return false;
	}

	app_fatal(StrCat("Unknown item size: ", itemSize.width, "x", itemSize.height));
}

bool AutoPlaceItemInInventorySlot(Player &player, int slotIndex, const Item &item, bool persistItem)
{
	int yy = (slotIndex > 0) ? (10 * (slotIndex / 10)) : 0;

	Size itemSize = GetInventorySize(item);
	for (int j = 0; j < itemSize.height; j++) {
		if (yy >= InventoryGridCells) {
			return false;
		}
		int xx = (slotIndex > 0) ? (slotIndex % 10) : 0;
		for (int i = 0; i < itemSize.width; i++) {
			if (xx >= 10 || player.InvGrid[xx + yy] != 0) {
				return false;
			}
			xx++;
		}
		yy += 10;
	}

	if (persistItem) {
		player.InvList[player._pNumInv] = item;
		player._pNumInv++;

		AddItemToInvGrid(player, slotIndex, player._pNumInv, itemSize);
		player.CalcScrolls();
	}

	return true;
}

int RoomForGold()
{
	int amount = 0;
	for (int8_t &itemIndex : MyPlayer->InvGrid) {
		if (itemIndex < 0) {
			continue;
		}
		if (itemIndex == 0) {
			amount += MaxGold;
			continue;
		}

		Item &goldItem = MyPlayer->InvList[itemIndex - 1];
		if (goldItem._itype != ItemType::Gold || goldItem._ivalue == MaxGold) {
			continue;
		}

		amount += MaxGold - goldItem._ivalue;
	}

	return amount;
}

int AddGoldToInventory(Player &player, int value)
{
	// Top off existing piles
	for (int i = 0; i < player._pNumInv && value > 0; i++) {
		Item &goldItem = player.InvList[i];
		if (goldItem._itype != ItemType::Gold || goldItem._ivalue >= MaxGold) {
			continue;
		}

		if (goldItem._ivalue + value > MaxGold) {
			value -= MaxGold - goldItem._ivalue;
			goldItem._ivalue = MaxGold;
		} else {
			goldItem._ivalue += value;
			value = 0;
		}

		NetSyncInvItem(player, i);
		SetPlrHandGoldCurs(goldItem);
	}

	// Last row right to left
	for (int i = 39; i >= 30 && value > 0; i--) {
		value = CreateGoldItemInInventorySlot(player, i, value);
	}

	// Remaining inventory in columns, bottom to top, right to left
	for (int x = 9; x >= 0 && value > 0; x--) {
		for (int y = 2; y >= 0 && value > 0; y--) {
			value = CreateGoldItemInInventorySlot(player, 10 * y + x, value);
		}
	}

	return value;
}

bool GoldAutoPlace(Player &player, Item &goldStack)
{
	goldStack._ivalue = AddGoldToInventory(player, goldStack._ivalue);
	SetPlrHandGoldCurs(goldStack);

	player._pGold = CalculateGold(player);

	return goldStack._ivalue == 0;
}

void CheckInvSwap(Player &player, inv_body_loc bLoc)
{
	Item &item = player.InvBody[bLoc];

	if (bLoc == INVLOC_HAND_LEFT && player.GetItemLocation(item) == ILOC_TWOHAND) {
		player.InvBody[INVLOC_HAND_RIGHT].clear();
	} else if (bLoc == INVLOC_HAND_RIGHT && player.GetItemLocation(item) == ILOC_TWOHAND) {
		player.InvBody[INVLOC_HAND_LEFT].clear();
	}

	CalcPlrInv(player, true);
}

void inv_update_rem_item(Player &player, inv_body_loc iv)
{
	player.InvBody[iv].clear();

	CalcPlrInv(player, player._pmode != PM_DEATH);
}

void CheckInvSwap(Player &player, const Item &item, int invGridIndex)
{
	auto itemSize = GetInventorySize(item);

	const int pitch = 10;
	int invListIndex = [&]() -> int {
		for (int y = 0; y < itemSize.height; y++) {
			int rowGridIndex = invGridIndex + pitch * y;
			for (int x = 0; x < itemSize.width; x++) {
				int gridIndex = rowGridIndex + x;
				if (player.InvGrid[gridIndex] != 0)
					return abs(player.InvGrid[gridIndex]);
			}
		}
		player._pNumInv++;
		return player._pNumInv;
	}();

	if (invListIndex < player._pNumInv) {
		for (auto &itemIndex : player.InvGrid) {
			if (itemIndex == invListIndex)
				itemIndex = 0;
			if (itemIndex == -invListIndex)
				itemIndex = 0;
		}
	}

	player.InvList[invListIndex - 1] = item;

	for (int y = 0; y < itemSize.height; y++) {
		int rowGridIndex = invGridIndex + pitch * y;
		for (int x = 0; x < itemSize.width; x++) {
			if (x == 0 && y == itemSize.height - 1)
				player.InvGrid[rowGridIndex + x] = invListIndex;
			else
				player.InvGrid[rowGridIndex + x] = -invListIndex;
		}
	}

	CalcPlrInv(player, true);
}

void CheckInvRemove(Player &player, int invGridIndex)
{
	int invListIndex = abs(player.InvGrid[invGridIndex]) - 1;

	if (invListIndex >= 0) {
		player.RemoveInvItem(invListIndex);
	}
}

void TransferItemToStash(Player &player, int location)
{
	if (location == -1) {
		return;
	}

	Item &item = GetInventoryItem(player, location);
	if (!AutoPlaceItemInStash(player, item, true)) {
		player.SaySpecific(HeroSpeech::WhereWouldIPutThis);
		return;
	}

	PlaySFX(ItemInvSnds[ItemCAnimTbl[item._iCurs]]);

	if (location < INVITEM_INV_FIRST) {
		RemoveEquipment(player, static_cast<inv_body_loc>(location), false);
		CalcPlrInv(player, true);
	} else if (location <= INVITEM_INV_LAST)
		player.RemoveInvItem(location - INVITEM_INV_FIRST);
	else
		player.RemoveSpdBarItem(location - INVITEM_BELT_FIRST);
}

void CheckInvItem(bool isShiftHeld, bool isCtrlHeld)
{
	if (!MyPlayer->HoldItem.isEmpty()) {
		CheckInvPaste(*MyPlayer, MousePosition);
	} else if (IsStashOpen && isCtrlHeld) {
		TransferItemToStash(*MyPlayer, pcursinvitem);
	} else {
		CheckInvCut(*MyPlayer, MousePosition, isShiftHeld, isCtrlHeld);
	}
}

void CheckInvScrn(bool isShiftHeld, bool isCtrlHeld)
{
	const Point mainPanelPosition = GetMainPanel().position;
	if (MousePosition.x > 190 + mainPanelPosition.x && MousePosition.x < 437 + mainPanelPosition.x
	    && MousePosition.y > mainPanelPosition.y && MousePosition.y < 33 + mainPanelPosition.y) {
		CheckInvItem(isShiftHeld, isCtrlHeld);
	}
}

void InvGetItem(Player &player, int ii)
{
	auto &item = Items[ii];
	if (dropGoldFlag) {
		CloseGoldDrop();
		dropGoldValue = 0;
	}

	if (dItem[item.position.x][item.position.y] == 0)
		return;

	item._iCreateInfo &= ~CF_PREGEN;
	CheckQuestItem(player, item);
	item.updateRequiredStatsCacheForPlayer(player);

	if (item._itype == ItemType::Gold && GoldAutoPlace(player, item)) {
		if (MyPlayer == &player) {
			// Non-gold items (or gold when you have a full inventory) go to the hand then provide audible feedback on
			//  paste. To give the same feedback for auto-placed gold we play the sound effect now.
			PlaySFX(IS_GOLD);
		}
	} else {
		// The item needs to go into the players hand
		if (MyPlayer == &player && !player.HoldItem.isEmpty()) {
			// drop whatever the player is currently holding
			NetSendCmdPItem(true, CMD_SYNCPUTITEM, player.position.tile, player.HoldItem);
		}

		// need to copy here instead of move so CleanupItems still has access to the position
		player.HoldItem = item;
		NewCursor(player.HoldItem);
	}

	// This potentially moves items in memory so must be done after we've made a copy
	CleanupItems(ii);
	pcursitem = -1;
}

std::optional<Point> FindAdjacentPositionForItem(Point origin, Direction facing)
{
	if (ActiveItemCount >= MAXITEMS)
		return {};

	if (CanPut(origin + facing))
		return origin + facing;

	if (CanPut(origin + Left(facing)))
		return origin + Left(facing);

	if (CanPut(origin + Right(facing)))
		return origin + Right(facing);

	if (CanPut(origin + Left(Left(facing))))
		return origin + Left(Left(facing));

	if (CanPut(origin + Right(Right(facing))))
		return origin + Right(Right(facing));

	if (CanPut(origin + Left(Left(Left(facing)))))
		return origin + Left(Left(Left(facing)));

	if (CanPut(origin + Right(Right(Right(facing)))))
		return origin + Right(Right(Right(facing)));

	if (CanPut(origin + Opposite(facing)))
		return origin + Opposite(facing);

	if (CanPut(origin))
		return origin;

	return {};
}

void AutoGetItem(Player &player, Item *itemPointer, int ii)
{
	Item &item = *itemPointer;

	if (dropGoldFlag) {
		CloseGoldDrop();
		dropGoldValue = 0;
	}

	if (dItem[item.position.x][item.position.y] == 0)
		return;

	item._iCreateInfo &= ~CF_PREGEN;
	CheckQuestItem(player, item);
	item.updateRequiredStatsCacheForPlayer(player);

	bool done;
	bool autoEquipped = false;

	if (item._itype == ItemType::Gold) {
		done = GoldAutoPlace(player, item);
		if (!done) {
			SetPlrHandGoldCurs(item);
		}
	} else {
		done = AutoEquipEnabled(player, item) && AutoEquip(player, item);
		if (done) {
			autoEquipped = true;
		}

		if (!done) {
			done = AutoPlaceItemInBelt(player, item, true);
		}
		if (!done) {
			done = AutoPlaceItemInInventory(player, item, true);
		}
	}

	if (done) {
		if (!autoEquipped && *sgOptions.Audio.itemPickupSound && &player == MyPlayer) {
			PlaySFX(IS_IGRAB);
		}

		CleanupItems(ii);
		return;
	}

	if (&player == MyPlayer) {
		player.Say(HeroSpeech::ICantCarryAnymore);
	}
	RespawnItem(item, true);
	NetSendCmdPItem(true, CMD_RESPAWNITEM, item.position, item);
}

int FindGetItem(int32_t iseed, _item_indexes idx, uint16_t createInfo)
{
	for (uint8_t i = 0; i < ActiveItemCount; i++) {
		auto &item = Items[ActiveItems[i]];
		if (item.keyAttributesMatch(iseed, idx, createInfo)) {
			return i;
		}
	}

	return -1;
}

void SyncGetItem(Point position, int32_t iseed, _item_indexes idx, uint16_t ci)
{
	// Check what the local client has at the target position
	int ii = dItem[position.x][position.y] - 1;

	if (ii >= 0 && ii < MAXITEMS) {
		// If there was an item there, check that it's the same item as the remote player has
		if (!Items[ii].keyAttributesMatch(iseed, idx, ci)) {
			// Key attributes don't match so we must've desynced, ignore this index and try find a matching item via lookup
			ii = -1;
		}
	}

	if (ii == -1) {
		// Either there's no item at the expected position or it doesn't match what is being picked up, so look for an item that matches the key attributes
		ii = FindGetItem(iseed, idx, ci);

		if (ii != -1) {
			// Translate to Items index for CleanupItems, FindGetItem returns an ActiveItems index
			ii = ActiveItems[ii];
		}
	}

	if (ii == -1) {
		// Still can't find the expected item, assume it was collected earlier and this caused the desync
		return;
	}

	CleanupItems(ii);
}

bool CanPut(Point position)
{
	if (!InDungeonBounds(position)) {
		return false;
	}

	if (IsTileSolid(position)) {
		return false;
	}

	if (dItem[position.x][position.y] != 0) {
		return false;
	}

	if (leveltype == DTYPE_TOWN) {
		if (dMonster[position.x][position.y] != 0) {
			return false;
		}
		if (dMonster[position.x + 1][position.y + 1] != 0) {
			return false;
		}
	}

	if (IsItemBlockingObjectAtPosition(position)) {
		return false;
	}

	return true;
}

int InvPutItem(const Player &player, Point position, const Item &item)
{
	std::optional<Point> itemTile = FindAdjacentPositionForItem(player.position.tile, GetDirection(player.position.tile, position));
	if (!itemTile)
		return -1;

	int ii = AllocateItem();

	dItem[itemTile->x][itemTile->y] = ii + 1;
	Items[ii] = item;
	Items[ii].position = *itemTile;
	RespawnItem(Items[ii], true);

	if (currlevel == 21 && *itemTile == CornerStone.position) {
		CornerStone.item = Items[ii];
		InitQTextMsg(TEXT_CORNSTN);
		Quests[Q_CORNSTN]._qlog = false;
		Quests[Q_CORNSTN]._qactive = QUEST_DONE;
	}

	return ii;
}

int SyncDropItem(Point position, _item_indexes idx, uint16_t icreateinfo, int iseed, int id, int dur, int mdur, int ch, int mch, int ivalue, uint32_t ibuff, int toHit, int maxDam, int minStr, int minMag, int minDex, int ac)
{
	if (ActiveItemCount >= MAXITEMS)
		return -1;

	int ii = AllocateItem();
	auto &item = Items[ii];

	dItem[position.x][position.y] = ii + 1;

	RecreateItem(*MyPlayer, item, idx, icreateinfo, iseed, ivalue, (ibuff & CF_HELLFIRE) != 0);
	if (id != 0)
		item._iIdentified = true;
	item._iDurability = dur;
	item._iMaxDur = mdur;
	item._iCharges = ch;
	item._iMaxCharges = mch;
	item._iPLToHit = toHit;
	item._iMaxDam = maxDam;
	item._iMinStr = minStr;
	item._iMinMag = minMag;
	item._iMinDex = minDex;
	item._iAC = ac;
	item.dwBuff = ibuff;
	item.position = position;
	RespawnItem(item, true);

	if (currlevel == 21 && position == CornerStone.position) {
		CornerStone.item = item;
		InitQTextMsg(TEXT_CORNSTN);
		Quests[Q_CORNSTN]._qlog = false;
		Quests[Q_CORNSTN]._qactive = QUEST_DONE;
	}
	return ii;
}

int SyncDropEar(Point position, uint16_t icreateinfo, int iseed, uint8_t cursval, string_view heroname)
{
	if (ActiveItemCount >= MAXITEMS)
		return -1;

	int ii = AllocateItem();
	auto &item = Items[ii];

	dItem[position.x][position.y] = ii + 1;
	RecreateEar(item, icreateinfo, iseed, cursval, heroname);
	item.position = position;
	RespawnItem(item, true);

	if (currlevel == 21 && position == CornerStone.position) {
		CornerStone.item = item;
		InitQTextMsg(TEXT_CORNSTN);
		Quests[Q_CORNSTN]._qlog = false;
		Quests[Q_CORNSTN]._qactive = QUEST_DONE;
	}
	return ii;
}

int8_t CheckInvHLight()
{
	auto newMousePosition = MousePosition;
	if (!MyPlayer->HoldItem.isEmpty()) {
		auto size = GetInvItemSize(pcurs);
		size.width += 2;
		size.height += 2;
		newMousePosition.x += size.width / 2;
		newMousePosition.y += size.height / 2;
	}
	int8_t r = 0;
	for (; r < NUM_XY_SLOTS; r++) {
		int xo = GetRightPanel().position.x;
		int yo = GetRightPanel().position.y;
		if (r >= SLOTXY_BELT_FIRST) {
			xo = GetMainPanel().position.x;
			yo = GetMainPanel().position.y;
		}

		if (newMousePosition.x >= InvRect[r].x + xo
		    && newMousePosition.x < InvRect[r].x + xo + (InventorySlotSizeInPixels.width + 1)
		    && newMousePosition.y >= InvRect[r].y + yo - (InventorySlotSizeInPixels.height + 1)
		    && newMousePosition.y < InvRect[r].y + yo) {
			break;
		}
	}

	if (r >= NUM_XY_SLOTS)
		return -1;

	int8_t rv = -1;
	InfoColor = UiFlags::ColorWhite;
	Item *pi = nullptr;
	Player &myPlayer = *MyPlayer;

	if (r >= SLOTXY_HEAD_FIRST && r <= SLOTXY_HEAD_LAST) {
		rv = INVLOC_HEAD;
		pi = &myPlayer.InvBody[rv];
	} else if (r == SLOTXY_RING_LEFT) {
		rv = INVLOC_RING_LEFT;
		pi = &myPlayer.InvBody[rv];
	} else if (r == SLOTXY_RING_RIGHT) {
		rv = INVLOC_RING_RIGHT;
		pi = &myPlayer.InvBody[rv];
	} else if (r == SLOTXY_AMULET) {
		rv = INVLOC_AMULET;
		pi = &myPlayer.InvBody[rv];
	} else if (r >= SLOTXY_HAND_LEFT_FIRST && r <= SLOTXY_HAND_LEFT_LAST) {
		rv = INVLOC_HAND_LEFT;
		pi = &myPlayer.InvBody[rv];
	} else if (r >= SLOTXY_HAND_RIGHT_FIRST && r <= SLOTXY_HAND_RIGHT_LAST) {
		pi = &myPlayer.InvBody[INVLOC_HAND_LEFT];
		if (pi->isEmpty() || myPlayer.GetItemLocation(*pi) != ILOC_TWOHAND) {
			rv = INVLOC_HAND_RIGHT;
			pi = &myPlayer.InvBody[rv];
		} else {
			rv = INVLOC_HAND_LEFT;
		}
	} else if (r >= SLOTXY_CHEST_FIRST && r <= SLOTXY_CHEST_LAST) {
		rv = INVLOC_CHEST;
		pi = &myPlayer.InvBody[rv];
	} else if (r >= SLOTXY_INV_FIRST && r <= SLOTXY_INV_LAST) {
		int8_t itemId = abs(myPlayer.InvGrid[r - SLOTXY_INV_FIRST]);
		if (itemId == 0)
			return -1;
		int ii = itemId - 1;
		rv = ii + INVITEM_INV_FIRST;
		pi = &myPlayer.InvList[ii];
	} else if (r >= SLOTXY_BELT_FIRST) {
		r -= SLOTXY_BELT_FIRST;
		RedrawComponent(PanelDrawComponent::Belt);
		pi = &myPlayer.SpdList[r];
		if (pi->isEmpty())
			return -1;
		rv = r + INVITEM_BELT_FIRST;
	}

	if (pi->isEmpty())
		return -1;

	if (pi->_itype == ItemType::Gold) {
		int nGold = pi->_ivalue;
		InfoString = fmt::format(fmt::runtime(ngettext("{:s} gold piece", "{:s} gold pieces", nGold)), FormatInteger(nGold));
	} else {
		InfoColor = pi->getTextColor();
		if (pi->_iIdentified) {
			InfoString = string_view(pi->_iIName);
			PrintItemDetails(*pi);
		} else {
			InfoString = string_view(pi->_iName);
			PrintItemDur(*pi);
		}
	}

	return rv;
}

void ConsumeScroll(Player &player)
{
	const SpellID spellId = player.executedSpell.spellId;

	const auto isCurrentSpell = [spellId](const Item &item) {
		return item.isScrollOf(spellId) || item.isRuneOf(spellId);
	};

	// Try to remove the scroll from selected inventory slot
	const int8_t itemSlot = player.executedSpell.spellFrom;
	if (itemSlot >= INVITEM_INV_FIRST && itemSlot <= INVITEM_INV_LAST) {
		const int itemIndex = itemSlot - INVITEM_INV_FIRST;
		const Item *item = &player.InvList[itemIndex];
		if (!item->isEmpty() && isCurrentSpell(*item)) {
			player.RemoveInvItem(itemIndex);
			return;
		}
	} else if (itemSlot >= INVITEM_BELT_FIRST && itemSlot <= INVITEM_BELT_LAST) {
		const int itemIndex = itemSlot - INVITEM_BELT_FIRST;
		const Item *item = &player.SpdList[itemIndex];
		if (!item->isEmpty() && isCurrentSpell(*item)) {
			player.RemoveSpdBarItem(itemIndex);
			return;
		}
	} else if (itemSlot != 0) {
		app_fatal(StrCat("ConsumeScroll: Invalid item index ", itemSlot));
	}

	// Didn't find it at the selected slot, take the first one we find
	// This path is always used when the scroll is consumed via spell selection
	RemoveInventoryOrBeltItem(player, isCurrentSpell);
}

bool CanUseScroll(Player &player, SpellID spell)
{
	if (leveltype == DTYPE_TOWN && !GetSpellData(spell).isAllowedInTown())
		return false;

	return HasInventoryOrBeltItem(player, [spell](const Item &item) {
		return item.isScrollOf(spell) || item.isRuneOf(spell);
	});
}

void ConsumeStaffCharge(Player &player)
{
	auto &staff = player.InvBody[INVLOC_HAND_LEFT];

	if (!CanUseStaff(staff, player.executedSpell.spellId))
		return;

	staff._iCharges--;
	CalcPlrStaff(player);
}

bool CanUseStaff(Player &player, SpellID spellId)
{
	return CanUseStaff(player.InvBody[INVLOC_HAND_LEFT], spellId);
}

Item &GetInventoryItem(Player &player, int location)
{
	if (location < INVITEM_INV_FIRST)
		return player.InvBody[location];

	if (location <= INVITEM_INV_LAST)
		return player.InvList[location - INVITEM_INV_FIRST];

	return player.SpdList[location - INVITEM_BELT_FIRST];
}

bool UseInvItem(size_t pnum, int cii)
{
	Player &player = Players[pnum];

	if (player._pInvincible && player._pHitPoints == 0 && &player == MyPlayer)
		return true;
	if (pcurs != CURSOR_HAND)
		return true;
	if (stextflag != TalkID::None)
		return true;
	if (cii < INVITEM_INV_FIRST)
		return false;

	bool speedlist = false;
	int c;
	Item *item;
	if (cii <= INVITEM_INV_LAST) {
		c = cii - INVITEM_INV_FIRST;
		item = &player.InvList[c];
	} else {
		if (talkflag)
			return true;
		c = cii - INVITEM_BELT_FIRST;

		item = &player.SpdList[c];
		speedlist = true;

		// If selected speedlist item exists in InvList, use the InvList item.
		for (int i = 0; i < player._pNumInv && *sgOptions.Gameplay.autoRefillBelt; i++) {
			if (player.InvList[i]._iMiscId == item->_iMiscId && player.InvList[i]._iSpell == item->_iSpell) {
				c = i;
				item = &player.InvList[c];
				speedlist = false;
				break;
			}
		}

		// If speedlist item is not inventory, use same item at the end of the speedlist if exists.
		if (speedlist && *sgOptions.Gameplay.autoRefillBelt) {
			for (int i = INVITEM_BELT_LAST - INVITEM_BELT_FIRST; i > c; i--) {
				Item &candidate = player.SpdList[i];

				if (!candidate.isEmpty() && candidate._iMiscId == item->_iMiscId && candidate._iSpell == item->_iSpell) {
					c = i;
					item = &candidate;
					break;
				}
			}
		}
	}

	constexpr int SpeechDelay = 10;
	if (item->IDidx == IDI_MUSHROOM) {
		player.Say(HeroSpeech::NowThatsOneBigMushroom, SpeechDelay);
		return true;
	}
	if (item->IDidx == IDI_FUNGALTM) {

		PlaySFX(IS_IBOOK);
		player.Say(HeroSpeech::ThatDidntDoAnything, SpeechDelay);
		return true;
	}

	if (player.isOnLevel(0)) {
		if (UseItemOpensHive(*item, player.position.tile)) {
			OpenHive();
			player.RemoveInvItem(c);
			return true;
		}
		if (UseItemOpensGrave(*item, player.position.tile)) {
			OpenGrave();
			player.RemoveInvItem(c);
			return true;
		}
	}

	if (!item->isUsable())
		return false;

	if (!player.CanUseItem(*item)) {
		player.Say(HeroSpeech::ICantUseThisYet);
		return true;
	}

	if (item->_iMiscId == IMISC_NONE && item->_itype == ItemType::Gold) {
		StartGoldDrop();
		return true;
	}

	if (dropGoldFlag) {
		CloseGoldDrop();
		dropGoldValue = 0;
	}

	if (item->isScroll() && leveltype == DTYPE_TOWN && !GetSpellData(item->_iSpell).isAllowedInTown()) {
		return true;
	}

	if (item->_iMiscId > IMISC_RUNEFIRST && item->_iMiscId < IMISC_RUNELAST && leveltype == DTYPE_TOWN) {
		return true;
	}

	int idata = ItemCAnimTbl[item->_iCurs];
	if (item->_iMiscId == IMISC_BOOK)
		PlaySFX(IS_RBOOK);
	else if (&player == MyPlayer)
		PlaySFX(ItemInvSnds[idata]);

	UseItem(pnum, item->_iMiscId, item->_iSpell);

	if (speedlist) {
		if (player.SpdList[c]._iMiscId == IMISC_NOTE) {
			InitQTextMsg(TEXT_BOOK9);
			CloseInventory();
			return true;
		}
		if (!item->isScroll() && !item->isRune())
			player.RemoveSpdBarItem(c);
		else
			player.queuedSpell.spellFrom = cii;
		return true;
	}
	if (player.InvList[c]._iMiscId == IMISC_MAPOFDOOM)
		return true;
	if (player.InvList[c]._iMiscId == IMISC_NOTE) {
		InitQTextMsg(TEXT_BOOK9);
		CloseInventory();
		return true;
	}
	if (!item->isScroll() && !item->isRune())
		player.RemoveInvItem(c);
	else
		player.queuedSpell.spellFrom = cii;

	return true;
}

void CloseInventory()
{
	CloseGoldWithdraw();
	IsStashOpen = false;
	invflag = false;
}

void DoTelekinesis()
{
	if (ObjectUnderCursor != nullptr)
		NetSendCmdLoc(MyPlayerId, true, CMD_OPOBJT, cursPosition);
	if (pcursitem != -1)
		NetSendCmdGItem(true, CMD_REQUESTAGITEM, MyPlayerId, pcursitem);
	if (pcursmonst != -1) {
		auto &monter = Monsters[pcursmonst];
		if (!M_Talker(monter) && monter.talkMsg == TEXT_NONE)
			NetSendCmdParam1(true, CMD_KNOCKBACK, pcursmonst);
	}
	NewCursor(CURSOR_HAND);
}

int CalculateGold(Player &player)
{
	int gold = 0;

	for (int i = 0; i < player._pNumInv; i++) {
		if (player.InvList[i]._itype == ItemType::Gold)
			gold += player.InvList[i]._ivalue;
	}

	return gold;
}

Size GetInventorySize(const Item &item)
{
	int itemSizeIndex = item._iCurs + CURSOR_FIRSTITEM;
	auto size = GetInvItemSize(itemSizeIndex);

	return { size.width / InventorySlotSizeInPixels.width, size.height / InventorySlotSizeInPixels.height };
}

} // namespace devilution
