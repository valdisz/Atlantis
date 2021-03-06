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
#include <map>
#include <algorithm>

FactBase::~FactBase() {

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

        ItemType& item = ItemDefs[soldier->race];

        if (item.flags & ItemType::MANPRODUCE) {
            this->fmi++;
            if (lost) this->fmiLost++;
            continue;
        }
        
        if (item.type & IT_UNDEAD) {
            this->undead++;
            if (lost) this->undeadLost++;
            continue;
        }

        if (item.type & IT_MONSTER) {
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

std::string EventLocation::getTerrain() {
    return TerrainDefs[this->terrainType].name;
}

void EventLocation::Assign(ARegion* region) {
	this->x = region->xloc;
	this->y = region->yloc;
	this->z = region->zloc;
	this->terrainType = region->type;
	this->province = region->name->Str();
    
    if (region->town) {
        this->settlement = region->town->name->Str();
        this->settlementType = region->town->TownType();
    }
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

bool compareEvents(const Event &first, const Event &second) {
    return first.score < second.score;
}


std::string Events::Write() {
    std::list<Event> events;

    // test start

    auto f = new BattleFact();
    f->defender.factionNum = 1;
    f->outcome = BATTLE_WON;
    this->AddFact(f);

    // test end

    for (auto &fact : this->facts) {
        fact->GetEvents(events);
    }

    std::map<EventCategory, std::vector<Event>> categories;
    for (auto &event : events) {
        if (categories.find(event.category) == categories.end()) {
            std::vector<Event> list = {};
            categories.insert(std::pair<EventCategory, std::vector<Event>>(event.category, list));
        }

        categories[event.category].push_back(event);
    }

    std::string text;
    for (auto &cat : categories) {
        auto list = cat.second;

        std::sort(list.begin(), list.end(), compareEvents);
        list.resize(std::min((int) list.size(), 10));

        int n = std::min((int) list.size(), getrandom(5) + 1);
        while (n-- > 0) {
            int i = getrandom(list.size());

            if (!text.empty()) text += "\n";
            text += list[i].text;
            
            list.erase(list.begin() + i);
        }
    }

    return text;
}
