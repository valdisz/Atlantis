// START A3HEADER
//
// This source file is part of the Atlantis PBM game program.
// Copyright (C) 2020 Valdis Zobēla
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

#ifndef EVENTS_CLASS
#define EVENTS_CLASS

#include "unit.h"
#include <string>
#include <list>

class Events;

class FactBase;
class BattleFact;


enum EventCategory {
    EVENT_BATTLE
};

struct Event {
    EventCategory category;
    int score;
    std::string text;
};


class FactBase {
public:
    virtual ~FactBase() = 0;
    virtual void GetEvents(std::list<Event> &events) = 0;
};

struct BattleSide {
    BattleSide();

    int factionNum;
    std::string factionName;

    int unitNum;
    std::string unitName;

    int total;
    int mages;
    int monsters;
    int fmi;
    int lost;
    int magesLost;
    int fmiLost;
    int monstersLost;

    void AssignUnit(Unit* unit);
    void AssignArmy(Army* army);
};

class BattleFact : public FactBase {
public:
    ~BattleFact();

    void GetEvents(std::list<Event> &events);

    int x;
    int y;
    int z;
    std::string province;
    std::string terrain;

    BattleSide attacker;
    BattleSide defender;
};

class Events {
public:
    Events();
    ~Events();

    std::string& Write();
    void AddFact(FactBase *fact);

private:
    std::list<FactBase *> facts;
};

#endif
