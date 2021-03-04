// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 2020 Valdi Zobēla
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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

#include "gameio.h"
#include "events.h"

BattleSide::BattleSide() {
    this->factionNum = 0;
    this->unitNum = 0;
    this->total = 0;
    this->mages = 0;
    this->monsters = 0;
    this->lost = 0;
    this->fmi = 0;
    this->fmiLost = 0;
    this->magesLost = 0;
    this->monstersLost = 0;
}

void BattleSide::AssignUnit(Unit* unit) {
	this->factionName = unit->faction->name->Str();
	this->factionNum = unit->faction->num;
	this->unitName = unit->name->Str();
	this->unitNum = unit->num;
}

void BattleSide::AssignArmy(Army* army) {
	this->total = army->count;
	for (int i = 0; i < army->count; i++) {
		auto soldier = army->soldiers[i];
        bool lost = soldier->hits == 0;

        if (lost) this->lost++;

        if (ItemDefs[soldier->race].flags & ItemType::MANPRODUCE) {
            this->fmi++;
            if (lost) this->fmiLost++;
            continue;
        }

        if (ItemDefs[soldier->race].type & IT_MONSTER) {
            this->monsters++;
            if (lost) this->monstersLost++;
            continue;
        }

        if (soldier->unit->type == U_MAGE || soldier->unit->type == U_GUARDMAGE) {
            this->mages++;
            if (lost) this->magesLost++;
            continue;
        }
	}
}

BattleFact::~BattleFact() {

}

const int N_MESSAGES = 4;
const char* ENCOUNTER[N_MESSAGES] = {
    "A small encounter between hostile forces",
    "A small encounter between hostile forces",
    "A small encounter between hostile forces",
    "A small encounter between hostile forces"
};

void BattleFact::GetEvents(std::list<Event> &events) {
    int total = this->attacker.total + this->defender.total;
    int totalLost = this->attacker.lost + this->defender.lost;
    int totalMages = this->attacker.mages + this->defender.mages;
    int totalMagesLost = this->attacker.magesLost + this->defender.magesLost;
    int totalMonsters = this->attacker.monsters + this->defender.monsters;
    int totalFMI = this->attacker.fmi + this->defender.fmi;

    int score = 0;
    std::string text;

    if (total <= 100) {
        // encounter

        std::string loses = totalLost > total / 2
            ? " small loses"
            : " big loses";

        text = std::string(ENCOUNTER[getrandom(N_MESSAGES)])
        + " in the " + this->terrain
        + " of " + this->province
        + " resulted in " + loses
        + ".";

        score = 1;
    }
    else if (total <= 500) {
        // local conflict

        std::string loses = totalLost > total / 2
            ? " small loses"
            : " big loses";


        text = std::string("A small encounter between hostile forces") +
        " in the " + this->terrain + " of " + this->province +
        " resulted in " + loses + ".";

        score = 2;
    }
    else if (total <= 2500) {
        // regional conflict
        score = 3;
    }
    else {
        // continental conflict
        score = 5;
    }

    if (totalMages > 0) score *= 2;
    if (totalMonsters > 0) score += 1;
    if (totalFMI > 0) score += 1;

    events.push_back({ EventCategory::EVENT_BATTLE, score, text });
}


/////-----


Events::Events() {

}

Events::~Events() {
    for (auto &fact : this->facts) {
        delete fact;
    }

    this->facts.clear();
}

void Events::AddFact(FactBase *fact) {
    this->facts.push_back(fact);
}

std::string& Events::Write() {
    std::list<Event> events;
    for (auto &fact : this->facts) {
        fact->GetEvents(events);
    }

    std::string text;
    for (auto &event : events) {
        text = text + "\n" + event.text;
    }

    return text;
}
