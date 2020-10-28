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
#ifndef ARMY_CLASS
#define ARMY_CLASS

#include <functional>
#include <map>
using namespace std;

class Army;

#include "unit.h"
#include "alist.h"
#include "items.h"
#include "object.h"
#include "shields.h"
#include "helper.h"
#include "soldier.h"

class Army
{
	public:
		Army(Unit *,AList *,int,int = 0);
		~Army();

		void WriteLosses(Battle *);
		void Lose(Battle *,ItemList *);
		void Win(Battle *,ItemList *);
		void Tie(Battle *);
		int CanBeHealed();
		void DoHeal(Battle *);
		void DoHealLevel(Battle *, int level, int rate, int useItems);
		void Regenerate(Battle *);

		void GetMonSpoils(ItemList *,int, int);

		int Broken();
		int NumAlive();
		int NumBehind();
		int NumSpoilers();
		int CanAttack();
		int NumFront();
		int NumFrontHits();
		Soldier *GetAttacker( int, int & );
		int GetEffectNum(char const *effect);
		int GetTargetNum(char const *special = NULL, int canAttackBehind = 0);
		Soldier *GetTarget( int );
		int RemoveEffects(int num, char const *effect);
		int DoAnAttack(Battle *, char const *special, int numAttacks, int attackType,
				int attackLevel, int flags, int weaponClass, char const *effect,
				int mountBonus, Soldier *attacker, Army *attackers, int attackbehind);
		void Kill(int);
		void Reset();

		//
		// These funcs are in specials.cpp
		//
		int CheckSpecialTarget(char const *,int);

		SoldierPtr * soldiers;
		Unit * leader;
		ShieldList shields;
		int round;
		int tac;
		int canfront;
		int canbehind;
		int notfront;
		int notbehind;
		int count;
		// Used if set ADVANCED_TACTICS
		int tactics_bonus;

		int hitsalive; // current number of "living hits"
		int hitstotal; // Number of hits at start of battle.
};

#endif
