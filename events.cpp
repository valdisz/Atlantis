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

#include "events.h"

BattleFact::~BattleFact() {

}

void BattleFact::GetEvents(std::list<Event> &events) {
    int total = this->attackers + this->defenders;
    int totalLost = this->attackersLost + this->defendersLost;
    int totalMages = this->attackerMages + this->defenderMages;
    int totalMagesLost = this->attackerMagesLost + this->defenderMagesLost;

    bool withMagic = totalMages > 0;

    int score = 1;
    std::string text;

    if (total <= 100) {
        // encounter

        std::string loses = totalLost > total / 2
            ? " small loses"
            : " big loses";

        text = std::string("A small encounter between hostile forces") +
        " in the land of " + this->province +
        " resulted in " + loses + ".";
    }
    else if (total <= 500) {
        // local conflict
    }
    else if (total <= 2500) {
        // regional conflict
    }
    else {
        // continental conflict
    }

    if (withMagic) {
        score *= 2;
    }

    events.push_back({ EventCategory::BATTLE, score, text });
}


/////-----


Events::Events() {

}

Events::~Events() {
    this->facts.clear();
}

void Events::AddFact(FactBase &fact) {
    this->facts.push_back(fact);
}

std::string Events::Write() {
    std::list<Event> events;
    for (auto &fact : this->facts) {
        fact.GetEvents(events);
    }
}
