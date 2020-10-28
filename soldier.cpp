// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 1995-1999 Geoff Dunbar
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program, in the file license.txt. If not, write
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// See the Atlantis Project web page for details:
// http://www.prankster.com/project
//
// END A3HEADER
#include "soldier.h"
#include "gameio.h"
#include "gamedata.h"

enum {
	WIN_NO_DEAD,
	WIN_DEAD,
	LOSS
};

Soldier::Soldier(Unit * u,Object * o,int regtype,int r,int ass)
{
	AString abbr;
	int i, item, armorType;

	race = r;
	unit = u;
	building = 0;

	healing = 0;
	healtype = 0;
	healitem = -1;
	canbehealed = 1;
	regen = 0;

	armor = -1;
	riding = -1;
	weapon = -1;

	attacks = 1;
	attacktype = ATTACK_COMBAT;

	special = NULL;
	slevel = 0;

	askill = 0;

	dskill[ATTACK_COMBAT] = 0;
	dskill[ATTACK_ENERGY] = -2;
	dskill[ATTACK_SPIRIT] = -2;
	dskill[ATTACK_WEATHER] = -2;
	dskill[ATTACK_RIDING] = 0;
	dskill[ATTACK_RANGED] = 0;
	for (int i=0; i<NUM_ATTACK_TYPES; i++)
		protection[i] = 0;
	damage = 0;
	hits = unit->GetAttribute("toughness");
	if (hits < 1) hits = 1;
	maxhits = hits;
	amuletofi = 0;
	battleItems = 0;

	/* Special case to allow protection from ships */
	if (o->IsFleet() && o->capacity < 1 && o->shipno < o->ships.Num()) {
		int objectno;

		i = 0;
		forlist(&o->ships) {
			Item *ship = (Item *) elem;
			if (o->shipno == i) {
				abbr = ItemDefs[ship->type].name;
				objectno = LookupObject(&abbr);
				if (objectno >= 0 && ObjectDefs[objectno].protect > 0) {
					o->capacity = ObjectDefs[objectno].protect * ship->num;
					o->type = objectno;
				}
				o->shipno++;
			}
			i++;
			if (o->capacity > 0) break;
		}
	}
	/* Building bonus */
	if (o->capacity) {
		building = o->type;
		//should the runes spell be a base or a bonus?
		for (int i=0; i<NUM_ATTACK_TYPES; i++) {
			if (Globals->ADVANCED_FORTS) {
				protection[i] += ObjectDefs[o->type].defenceArray[i];
			} else
				dskill[i] += ObjectDefs[o->type].defenceArray[i];
		}
		if (o->runes) {
			dskill[ATTACK_ENERGY] = max(dskill[ATTACK_ENERGY], o->runes);
			dskill[ATTACK_SPIRIT] = max(dskill[ATTACK_SPIRIT], o->runes);
		}
		o->capacity--;
	}

	/* Is this a monster? */
	if (ItemDefs[r].type & IT_MONSTER) {
		MonType *mp = FindMonster(ItemDefs[r].abr,
				(ItemDefs[r].type & IT_ILLUSION));
		if((u->type == U_WMON) || (ItemDefs[r].flags & ItemType::MANPRODUCE))
			name = AString(mp->name) + " in " + *(unit->name);
		else
			name = AString(mp->name) + " controlled by " + *(unit->name);
		askill = mp->attackLevel;
		dskill[ATTACK_COMBAT] += mp->defense[ATTACK_COMBAT];
		if (mp->defense[ATTACK_ENERGY] > dskill[ATTACK_ENERGY]) {
			dskill[ATTACK_ENERGY] = mp->defense[ATTACK_ENERGY];
		}
		if (mp->defense[ATTACK_SPIRIT] > dskill[ATTACK_SPIRIT]) {
			dskill[ATTACK_SPIRIT] = mp->defense[ATTACK_SPIRIT];
		}
		if (mp->defense[ATTACK_WEATHER] > dskill[ATTACK_WEATHER]) {
			dskill[ATTACK_WEATHER] = mp->defense[ATTACK_WEATHER];
		}
		dskill[ATTACK_RIDING] += mp->defense[ATTACK_RIDING];
		dskill[ATTACK_RANGED] += mp->defense[ATTACK_RANGED];
		damage = 0;
		hits = mp->hits;
		if (hits < 1) hits = 1;
		maxhits = hits;
		attacks = mp->numAttacks;
		if (!attacks) attacks = 1;
		special = mp->special;
		slevel = mp->specialLevel;
		if (Globals->MONSTER_BATTLE_REGEN) {
			regen = mp->regen;
			if (regen < 0) regen = 0;
		}
		return;
	}

	name = *(unit->name);

	SetupHealing();

	SetupSpell();
	SetupCombatItems();

	// Set up armor
	for (i = 0; i < MAX_READY; i++) {
		// Check preferred armor first.
		item = unit->readyArmor[i];
		if (item == -1) break;
		abbr = ItemDefs[item].abr;
		item = unit->GetArmor(abbr, ass);
		if (item != -1) {
			armor = item;
			break;
		}
	}
	if (armor == -1) {
		for (armorType = 1; armorType < NUMARMORS; armorType++) {
			abbr = ArmorDefs[armorType].abbr;
			item = unit->GetArmor(abbr, ass);
			if (item != -1) {
				armor = item;
				break;
			}
		}
	}

	//
	// Check if this unit is mounted
	//
	int terrainflags = TerrainDefs[regtype].flags;
	int canFly = (terrainflags & TerrainType::FLYINGMOUNTS);
	int canRide = (terrainflags & TerrainType::RIDINGMOUNTS);
	int ridingBonus = 0;
	if (canFly || canRide) {
		//
		// Mounts of some type _are_ allowed in this region
		//
		int mountType;
		if (ItemDefs[race].type & IT_MOUNT) {
			// If the man is a mount (Centaurs), then the only option
			// they have for riding is the built-in one
			abbr = ItemDefs[race].abr;
			item = unit->GetMount(abbr, canFly, canRide, ridingBonus);
		} else {
			for (mountType = 1; mountType < NUMMOUNTS; mountType++) {
				abbr = MountDefs[mountType].abbr;
				// See if this mount is an option
				item = unit->GetMount(abbr, canFly, canRide, ridingBonus);
				if (item == -1) continue;
				// No riding other men in combat
				if (ItemDefs[item].type & IT_MAN) {
					item = -1;
					ridingBonus = 0;
					continue;
				}
				break;
			}
		}
		// Defer adding the combat bonus until we know if the weapon
		// allows it.  The defense bonus for riding can be added now
		// however.
		dskill[ATTACK_RIDING] += ridingBonus;
		riding = item;
	}

	//
	// Find the correct weapon for this soldier.
	//
	int weaponType;
	int attackBonus = 0;
	int defenseBonus = 0;
	int numAttacks = 1;
	for (i = 0; i < MAX_READY; i++) {
		// Check the preferred weapon first.
		item = unit->readyWeapon[i];
		if (item == -1) break;
		abbr = ItemDefs[item].abr;
		item = unit->GetWeapon(abbr, riding, ridingBonus, attackBonus,
				defenseBonus, numAttacks);
		if (item != -1) {
			weapon = item;
			break;
		}
	}
	if (weapon == -1) {
		for (weaponType = 1; weaponType < NUMWEAPONS; weaponType++) {
			abbr = WeaponDefs[weaponType].abbr;
			item = unit->GetWeapon(abbr, riding, ridingBonus, attackBonus,
					defenseBonus, numAttacks);
			if (item != -1) {
				weapon = item;
				break;
			}
		}
	}
	// If we did not get a weapon, set attack and defense bonuses to
	// combat skill (and riding bonus if applicable).
	if (weapon == -1) {
		attackBonus = unit->GetAttribute("combat") + ridingBonus;
		defenseBonus = attackBonus;
		numAttacks = 1;
	} else {
		// Okay.  We got a weapon.  If this weapon also has a special
		// and we don't have a special set, use that special.
		// Weapons (like Runeswords) which are both weapons and battle
		// items will be skipped in the battle items setup and handled
		// here.
		if ((ItemDefs[weapon].type & IT_BATTLE) && special == NULL) {
			BattleItemType *pBat = FindBattleItem(ItemDefs[weapon].abr);
			special = pBat->special;
			slevel = pBat->skillLevel;
		}
	}

	unit->PracticeAttribute("combat");

	// Set the attack and defense skills
	// These will include the riding bonus if they should be included.
	askill += attackBonus;
	dskill[ATTACK_COMBAT] += defenseBonus;
	attacks = numAttacks;
}

void Soldier::SetupSpell()
{
	if (unit->type != U_MAGE && unit->type != U_GUARDMAGE) return;

	if (unit->combat != -1) {
		slevel = unit->GetSkill(unit->combat);
		if (!slevel) {
			//
			// The unit can't cast this spell!
			//
			unit->combat = -1;
			return;
		}

		SkillType *pST = &SkillDefs[unit->combat];
		if (!(pST->flags & SkillType::COMBAT)) {
			//
			// This isn't a combat spell!
			//
			unit->combat = -1;
			return;
		}

		special = pST->special;
		unit->Practice(unit->combat);
	}
}

void Soldier::SetupCombatItems()
{
	int battleType;
	int exclusive = 0;

	for (battleType = 1; battleType < NUMBATTLEITEMS; battleType++) {
		BattleItemType *pBat = &BattleItemDefs[battleType];

		AString abbr = pBat->abbr;
		int item = unit->GetBattleItem(abbr);
		if (item == -1) continue;

		// If we are using the ready command, skip this item unless
		// it's the right one, or unless it is a shield which doesn't
		// need preparing.
		if (!Globals->USE_PREPARE_COMMAND ||
				((unit->readyItem == -1) &&
				 (Globals->USE_PREPARE_COMMAND == GameDefs::PREPARE_NORMAL)) ||
				(item == unit->readyItem) ||
				(pBat->flags & BattleItemType::SHIELD)) {
			if ((pBat->flags & BattleItemType::SPECIAL) && special != NULL) {
				// This unit already has a special attack so give the item
				// back to the unit as they aren't going to use it.
				unit->items.SetNum(item, unit->items.GetNum(item)+1);
				continue;
			}
			if (pBat->flags & BattleItemType::MAGEONLY &&
					unit->type != U_MAGE && unit->type != U_GUARDMAGE &&
					unit->type != U_APPRENTICE) {
				// Only mages/apprentices can use this item so give the
				// item back to the unit as they aren't going to use it.
				unit->items.SetNum(item, unit->items.GetNum(item)+1);
				continue;
			}

			if (pBat->flags & BattleItemType::EXCLUSIVE) {
				if (exclusive) {
					// Can only use one exclusive item, and we already
					// have one, so give the extras back.
					unit->items.SetNum(item, unit->items.GetNum(item)+1);
					continue;
				}
				exclusive = 1;
			}

			if (pBat->flags & BattleItemType::MAGEONLY) {
				unit->Practice(S_MANIPULATE);
			}

			/* Make sure amulets of invulnerability are marked */
			if (item == I_AMULETOFI) {
				amuletofi = 1;
			}

			SET_BIT(battleItems, battleType);

			if (pBat->flags & BattleItemType::SPECIAL) {
				special = pBat->special;
				slevel = pBat->skillLevel;
			}

			if (pBat->flags & BattleItemType::SHIELD) {
				SpecialType *sp = FindSpecial(pBat->special);
				for (int i = 0; i < 4; i++) {
					if (sp->shield[i] == NUM_ATTACK_TYPES) {
						for (int j = 0; j < NUM_ATTACK_TYPES; j++) {
							if (dskill[j] < pBat->skillLevel)
								dskill[j] = pBat->skillLevel;
						}
					} else if (sp->shield[i] >= 0) {
						if (dskill[sp->shield[i]] < pBat->skillLevel)
							dskill[sp->shield[i]] = pBat->skillLevel;
					}
				}
			}
		} else {
			// We are using prepared items and this item is NOT the one
			// we have prepared, so give it back to the unit as they won't
			// use it.
			unit->items.SetNum(item, unit->items.GetNum(item)+1);
			continue;
		}
	}
}

int Soldier::HasEffect(char const *eff)
{
	if (eff == NULL) return 0;

	return effects[eff];
}

void Soldier::SetEffect(char const *eff)
{
	if (eff == NULL) return;
	int i;

	EffectType *e = FindEffect(eff);
	if (e == NULL) return;

	askill += e->attackVal;

	for (i = 0; i < 4; i++) {
		if (e->defMods[i].type != -1)
			dskill[e->defMods[i].type] += e->defMods[i].val;
	}

	if (e->cancelEffect != NULL) ClearEffect(e->cancelEffect);

	if (!(e->flags & EffectType::EFF_NOSET)) effects[eff] = 1;
}

void Soldier::ClearEffect(char const *eff)
{
	if (eff == NULL) return;
	int i;

	EffectType *e = FindEffect(eff);
	if (e == NULL) return;

	askill -= e->attackVal;

	for (i = 0; i < 4; i++) {
		if (e->defMods[i].type != -1)
			dskill[e->defMods[i].type] -= e->defMods[i].val;
	}

	effects[eff] = 0;
}

void Soldier::ClearOneTimeEffects(void)
{
	for (int i = 0; i < NUMEFFECTS; i++) {
		if (HasEffect(EffectDefs[i].name) &&
				(EffectDefs[i].flags & EffectType::EFF_ONESHOT))
			ClearEffect(EffectDefs[i].name);
	}
}

int Soldier::ArmorProtect(int weaponClass)
{
	//
	// Return 1 if the armor is successful
	//
	ArmorType *pArm = NULL;
	if (armor > 0) pArm = FindArmor(ItemDefs[armor].abr);
	if (pArm == NULL) return 0;
	int chance = pArm->saves[weaponClass];

	if (chance <= 0) return 0;
	if (chance > getrandom(pArm->from)) return 1;

	return 0;
}

void Soldier::RestoreItems()
{
	if (healing && healitem != -1) {
		if (healitem == I_HERBS) {
			unit->items.SetNum(healitem,
					unit->items.GetNum(healitem) + healing);
		} else if (healitem == I_HEALPOTION) {
			unit->items.SetNum(healitem,
					unit->items.GetNum(healitem)+1);
		}
	}
	if (weapon != -1)
		unit->items.SetNum(weapon,unit->items.GetNum(weapon) + 1);
	if (armor != -1)
		unit->items.SetNum(armor,unit->items.GetNum(armor) + 1);
	if (riding != -1 && !(ItemDefs[riding].type & IT_MAN))
		unit->items.SetNum(riding,unit->items.GetNum(riding) + 1);

	int battleType;
	for (battleType = 1; battleType < NUMBATTLEITEMS; battleType++) {
		BattleItemType *pBat = &BattleItemDefs[ battleType ];

		if (GET_BIT(battleItems, battleType)) {
			AString itm(pBat->abbr);
			int item = LookupItem(&itm);
			unit->items.SetNum(item, unit->items.GetNum(item) + 1);
		}
	}
}

void Soldier::Alive(int state)
{
	RestoreItems();

	if (state == LOSS) {
		unit->canattack = 0;
		unit->routed = 1;
		/* Guards with amuletofi will not go off guard */
		if (!amuletofi &&
			(unit->guard == GUARD_GUARD || unit->guard == GUARD_SET)) {
			unit->guard = GUARD_NONE;
		}
	} else {
		unit->advancefrom = 0;
	}

	if (state == WIN_DEAD) {
		unit->canattack = 0;
		unit->nomove = 1;
	}
}

void Soldier::Dead()
{
	RestoreItems();

	unit->SetMen(race,unit->GetMen(race) - 1);
}
