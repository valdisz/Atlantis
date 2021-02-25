#include <stdio.h>
#include <string.h>
#include "game.h"
#include "gamedata.h"
#include "aregion.h"

#include <algorithm>
#include <list>
#include <unordered_set>
#include <stack>

int makeRoll(int rolls, int sides) {
	int result = 0;
	for (int i = 0; i < rolls; i++) {
		result += getrandom(sides) + 1;
	}

	return result;
}

int clamp(int min, int value, int max) {
	return std::max(min, std::min(value, max));
}

int hexDistance(const Coords& a, const Coords& b) {
    int qA = a.x;
    int rA = (a.y - a.x) / 2;
    int sA = -qA - rA;

    int qB = b.x;
    int rB = (b.y - b.x) / 2;
    int sB = -qB - rB;

	int q = qA - qB;
	int r = rA - rB;
	int s = sA - sB;

	int d = (std::abs(q) + std::abs(r) + std::abs(s)) / 2;
	return d;
}

int Coords::Distance(const Coords& other) {
	return hexDistance(*this, other);
}

std::map<int, int> ZoneRegion::CountNeighborBiomes() {
	std::map<int, int> stats;
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;

		if (stats.find(n->biome) == stats.end()) {
			stats[n->biome] = 1;
		}
		else {
			stats[n->biome] += 1;
		}
	}

	return stats;
}

int ZoneRegion::CountNeighbors(Zone* zone) {
	int count = 0;
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n != NULL && n->zone == zone) count++;
	}

	return count;
}

int ZoneRegion::CountNeighbors(Province* province) {
	int count = 0;
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n != NULL && n->province == province) count++;
	}

	return count;
}

std::vector<Zone *> ZoneRegion::GetNeihborZones() {
	std::vector<Zone *> items;
	items.reserve(NDIRS);
	
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;
		if (n->zone == zone) continue;

		auto z = n->zone;
		for (int j = 0; j < items.size(); j++) {
			if (items[j] == z) {
				z = NULL;
				break;
			}
		}

		if (z != NULL) items.push_back(z);
	}

	return items;
}

bool ZoneRegion::HaveBorderWith(Zone* zone) {
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;
		if (n->zone == zone) return true;
	}

	return false;
}

bool ZoneRegion::HaveBorderWith(ZoneRegion* other) {
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;
		if (n == other) return true;
	}

	return false;
}

bool ZoneRegion::IsInner() {
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;
		if (n->zone != this->zone) return false;
	}

	return true;
}

bool ZoneRegion::IsOuter() {
	return !IsInner();
}




std::unordered_set<Province *> Province::GetNeighbors() {
	std::unordered_set<Province *> items;

	for (auto &kv : this->regions) {
		auto reg = kv.second;

		for (int i = 0; i < NDIRS; i++) {
			auto n = reg->neighbors[i];
			if (n == NULL) continue;

			if (n->zone == this->zone && n->province != this) {
				items.insert(n->province);
			}
		}
	}

	return items;
}

std::unordered_set<int> Province::GetNeighborBiomes() {
	std::unordered_set<int> biomes;
	
	auto tmp = this->GetNeighbors();
	for (auto &n : tmp) {
		if (n->biome != -1) biomes.insert(n->biome);
	}

	return biomes;
}

Coords Province::GetLocation() {
	int xMin = -1;
	int yMin;
	int xMax;
	int yMax;

	for (auto &kv : regions) {
		auto reg = kv.second;

		if (xMin == -1) {
			xMin = reg->location.x;
			yMin = reg->location.y;
			xMax = xMin;
			yMax = yMin;

			continue;
		}
		
		xMin = std::min(xMin, reg->location.x);
		yMin = std::min(yMin, reg->location.y);
		xMax = std::max(xMax, reg->location.x);
		yMax = std::max(yMax, reg->location.y);
	}

	return {
		x: xMin + (xMax - xMin + 1) / 2,
		y: yMin + (yMax - yMin + 1) / 2
	};
}

int Province::GetLatitude() {
	auto loc = GetLocation();
	int lat = ( loc.y * 8 ) / this->h;
	if (lat > 3) {
		lat = std::max(0, 7 - lat);
	}
}

int Province::GetSize() {
	return this->regions.size();
}

void Province::AddRegion(ZoneRegion* region) {
	region->province = this;
	this->regions.insert(std::make_pair(region->id, region));
}

void Province::RemoveRegion(ZoneRegion* region) {
	if (region->province == this) {
		region->province = NULL;
	}

	this->regions.erase(region->id);
}

bool Province::Grow() {
	std::vector<ZoneRegion *> candidates;
	for (auto &kv : regions) {
		auto reg = kv.second;

		for (int i = 0; i < NDIRS; i++) {
			auto n = reg->neighbors[i];
			if (n == NULL) continue;
			if (n->zone != this->zone) continue;
			if (n->province != NULL) continue;

			candidates.push_back(n);
		}
	}

	if (candidates.size() == 0) return false;

	ZoneRegion* next = candidates[getrandom(candidates.size())];
	int connections = next->CountNeighbors(this);

	// 2d6
	int roll = makeRoll(2, 6);
	int diff = 12 - connections;

	// connections: 5, diff: 7, prob: 58%
	// connections: 4, diff: 8, prob: 41%
	// connections: 3, diff: 9, prob: 27%
	// connections: 2, diff: 10, prob: 16%
	// connections: 1, diff: 11, prob: 8%

	if (GetSize() == 1 || roll >= diff) {
		AddRegion(next);
	}

	return true;
}

Zone::~Zone() {
	for (auto &kv : provinces) {
		delete kv.second;
	}
	provinces.clear();
}

Province* Zone::CreateProvince(ZoneRegion* region, int h) {
	Province* province = new Province;
	province->id = this->provinces.size();
	province->zone = this;
	province->h = h;
	province->biome = -1;

	province->AddRegion(region);
	this->provinces.insert(std::make_pair(province->id, province));

	return province;
}

void Zone::RemoveProvince(Province* province) {
	if (province->zone == this) {
		province->zone = NULL;
	}

	this->provinces.erase(province->id);

	delete province;
}

bool ZoneRegion::AtBorderWith(ZoneType type) {
	for (int i = 0; i < NDIRS; i++) {
		auto n = this->neighbors[i];
		if (n == NULL) continue;
		if (n->zone != this->zone && n->zone->type == type) return true;
	}

	return false;
}

bool Zone::IsIsland() {
	if (this->type != ZoneType::CONTINENT) return false;

	for (auto &kv : this->neighbors) {
		auto zone = kv.second;
		if (zone->type == ZoneType::CONTINENT) return false;
	}

	return true;
}

void Zone::RemoveRegion(ZoneRegion* region) {
	if (region->zone == this) {
		region->zone = NULL;
	}

	this->regions.erase(region->id);
}

void Zone::AddRegion(ZoneRegion* region) {
	if (region == NULL) {
		Awrite("Region cannot be NULL");
		exit(1);
	}

	region->zone = this;
	this->regions.insert(std::make_pair(region->id, region));

	if (this->regions.size() == 1) {
		this->location = region->location;
	}
}

void Zone::RemoveNeighbor(Zone* zone) {
	zone->neighbors.erase(this->id);
	this->neighbors.erase(zone->id);
}

void Zone::AddNeighbor(Zone* zone) {
	zone->neighbors.insert(std::make_pair(this->id, this));
	this->neighbors.insert(std::make_pair(zone->id, zone));
}

void Zone::SetConnections() {
	std::unordered_set<Zone *> visited;
	
	for (auto &region : this->regions) {
		for (auto &n : region.second->neighbors) {
			if (n == NULL) continue;
			if (n->zone == this) continue;

			if (n->zone == NULL) {
				Awrite("Region zone cannot be NULL");
				exit(1);
			}

			visited.insert(n->zone);
		}
	}

	this->neighbors.clear();
	for (auto &z : visited) {
		AddNeighbor(z);
	}
}

std::unordered_set<ZoneRegion *> Zone::TraverseRegions() {
	std::unordered_set<ZoneRegion *> v;
	std::stack<ZoneRegion *> s;

	if (this->regions.size() == 0) return v;

	s.push(this->regions.begin()->second);
	while (!s.empty()) {
		ZoneRegion* reg = s.top();
		s.pop();
		v.insert(reg);

		for (int i = 0; i < NDIRS; i++) {
			auto n = reg->neighbors[i];
			if (n == NULL) continue;
			if (n->zone != this) continue;
			if (v.find(n) != v.end()) continue;

			s.push(n);
		}
	}

	return v;
}

bool Zone::CheckZoneIntegerity() {
	if (this->regions.size() == 0) {
		return true;
	}

	auto v = this->TraverseRegions();

	return v.size() == this->regions.size();
}

bool Zone::AtBorderWith(Zone* zone) {
	for (auto &z : zone->neighbors) {
		if (this == z.second) return true;
	}

	return false;
}

int GetRegionIndex(const int x, const int y, const int w, const int h) {
	int xx = (x + w) % w;
	int yy = (y + h) % h;
	
	if ((xx + yy) % 2) {
		return -1;
	}

	int i = xx / 2 + yy * w / 2;

	return i;
}

Zone* MapBuilder::GetZone(int x, int y) {
    auto reg = this->GetRegion(x, y);
    if (reg == NULL) return NULL;

    return reg->zone;
}

Zone* MapBuilder::GetNotIsland() {
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		if (zone->type != ZoneType::CONTINENT) continue;

		if (!zone->IsIsland()) {
			return zone;
		}
	}

	return NULL;
}

Zone* MapBuilder::GetZoneOfMaxSize(ZoneType type, int maxSize) {
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		if (zone->type != type) continue;

		if (zone->regions.size() <= maxSize) {
			return zone;
		}
	}

	return NULL;
}

MapBuilder::MapBuilder(ARegionArray* aregs) {
	this->w = aregs->x;
	this->h = aregs->y;
	this->lastZoneId = 0;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			if (!((x + y) % 2)) {
				ZoneRegion *reg = new ZoneRegion;
				reg->id = GetRegionIndex(x, y, this->w, this->h);
				reg->location = { x: x, y: y };
				reg->zone = NULL;
				reg->exclude = false;
				reg->province = NULL;
				reg->region = aregs->GetRegion(x, y);
				reg->biome = -1;

				if (reg->location.x != reg->region->xloc || reg->location.y != reg->region->yloc) {
					Awrite("Region location do not match");
					exit(1);
				}

				this->regions.push_back(reg);
			}
		}
	}

	for (auto &item : this->regions) {
		ARegion* areg = item->region;
		for (int i = 0; i < NDIRS; i++) {
			ARegion* nreg = areg->neighbors[i];

			item->neighbors[i] = nreg == NULL
				? NULL
				: this->GetRegion(nreg->xloc, nreg->yloc);
		}
	}
}

Zone* MapBuilder::CreateZone(ZoneType type) {
	Zone* zone = new Zone;
	zone->id = this->lastZoneId++;
	zone->type = type;

	this->zones.insert(std::make_pair(zone->id, zone));

	return zone;
}

void MapBuilder::Clear() {
	for (auto &region : this->regions) {
		delete region;
	}

	for (auto &zone : this->zones) {
		delete zone.second;
	}

	this->regions.clear();
	this->zones.clear();
}

MapBuilder::~MapBuilder() {
	Clear();
}

void MapBuilder::ConnectZones() {
	for (auto &zone : this->zones) {
		zone.second->SetConnections();
	}
}

ZoneRegion* MapBuilder::GetRegion(const int x, const int y) {
	return this->regions[GetRegionIndex(x, y, this->w, this->h)];
}

ZoneRegion* MapBuilder::GetRegion(Coords location) {
	return this->GetRegion(location.x, location.y);
}

void MapBuilder::ClearEmptyZones() {
	int count = 0;
	std::vector<int> keys;
	for (auto &z : this->zones) {
		keys.push_back(z.first);
	}

	for (auto &key : keys) {
		Zone* zone = this->zones[key];
		if (zone->regions.size() == 0) {
			delete zone;
			this->zones.erase(key);
			count++;
		}
	}

	Awrite(AString("Removed ") + count + " empty zones");
}

void MapBuilder::CreateZones(int minDistance, int maxAtempts) {
	Awrite("Create zones");

	int attempts = 0;
	while (this->zones.size() < this->maxZones && attempts++ < maxAtempts) {
		Coords location = {
			x: getrandom(this->w),
			y: getrandom(this->h)
		};

		if ((location.x + location.y) % 2) continue;

		int distance = w + h;
		for (auto &kv : this->zones) {
			auto zone = kv.second;
			distance = std::min(distance, hexDistance(zone->location, location));
			if (distance < minDistance) break;
		}

		if (distance < minDistance) continue;

		Zone* z = CreateZone(ZoneType::UNDECIDED);
		ZoneRegion* r = this->GetRegion(location);
		z->AddRegion(r);

		attempts = 0;
	}
}

void MapBuilder::GrowZones() {
	Awrite("Grow zones");

	std::unordered_set<Zone *> v;

	// run loop while there are regions not assigned to any zone
	std::vector<ZoneRegion *> nextRegions;
	while (this->zones.size() != v.size()) {
		for (auto &kv : this->zones) {
			auto zone = kv.second;
			if (v.find(zone) != v.end()) {
				// already processed zone
				continue;
			}

			// find all regions where zone can grow
			nextRegions.clear();
			for (auto &kv2 : zone->regions) {
				auto region = kv2.second;
				for (auto &n : region->neighbors) {
					if (n == NULL) continue;
					if (n->zone != NULL) continue;

					nextRegions.push_back(n);
				}
			}

			if (nextRegions.size() == 0) {
				v.insert(zone);
				continue;
			}

			// select one region to grow
			ZoneRegion* next = nextRegions[getrandom(nextRegions.size())];
			int connections = 0;
			for (auto &n : next->neighbors) {
				if (n != NULL && n->zone == zone) {
					connections++;
				}
			}

			// 2d6
			int roll = makeRoll(2, 6);
			int diff = 12 - connections;

			// connections: 5, diff: 7, prob: 58%
			// connections: 4, diff: 8, prob: 41%
			// connections: 3, diff: 9, prob: 27%
			// connections: 2, diff: 10, prob: 16%
			// connections: 1, diff: 11, prob: 8%

			if (zone->regions.size() == 1 || roll >= diff) {
				zone->AddRegion(next);
			}
		}
	}

	ClearEmptyZones();
	ConnectZones();
} 

void MapBuilder::MergeZoneInto(Zone* src, Zone* dest) {
	if (src == NULL) {
		Awrite("src cannot be null");
		exit(1);
	}
	
	if (dest == NULL) {
		Awrite("dest cannot be null");
		exit(1);
	}

	if (!src->AtBorderWith(dest)) {
		Awrite("Zones must have common border");
		exit(1);
	}

	std::unordered_set<Zone *> conn;
	for (auto &kv : src->regions) {
		auto reg = kv.second;

		auto zones = reg->GetNeihborZones();
		for (auto &z : zones) {
			conn.insert(z);
		}

		dest->AddRegion(kv.second);
	}

	src->regions.clear();
	src->neighbors.clear();

	dest->SetConnections();
	for (auto &z : conn) {
		z->SetConnections();
	}
}

void MapBuilder::SplitZone(Zone* zone) {
	if (zone == NULL) {
		Awrite("Cannot split NULL zone");
		exit(1);
	}

	auto blob = zone->TraverseRegions();
	while (zone->regions.size() != blob.size()) {
		Zone* newZone = CreateZone(zone->type);
		
		for (auto &region : blob) {
			zone->RemoveRegion(region);
			newZone->AddRegion(region);
		}

		for (auto &z : zone->neighbors) {
			z.second->SetConnections();
		}

		zone->SetConnections();
		newZone->SetConnections();

		blob = zone->TraverseRegions();
	}
}

Zone* FindConnectedContinent(Zone* zone) {
	Awrite("Searching for continent");
	for (auto &kv : zone->neighbors) {
		auto n = kv.second;
		if (n->type == ZoneType::CONTINENT) {
			return n;
		}
	}

	return NULL;
}

std::vector<ZoneRegion *> FindBorderRegions(Zone* zone, Zone* borderZone, const int depth, const bool useRandom = false) {
	std::unordered_set<ZoneRegion *> visited;

	const int diff = 3;

	std::vector<ZoneRegion *> v;
	for (int i = 0; i < depth; i++) {
		if (i == 0) {
			for (auto &kv : zone->regions) {
				auto &region = kv.second;
				if (!region->HaveBorderWith(borderZone)) continue;

				if (useRandom && depth == 1) {
					int roll = makeRoll(1, 5);
					if (roll < diff) continue;
				}
				
				visited.insert(region);
			}

			continue;
		}

		v.clear();
		for (auto &region : visited) {
			for (auto &next : region->neighbors) {
				if (next == NULL) continue;
				if (next->zone != zone) continue;
				if (visited.find(next) != visited.end()) continue;

				if (i == depth - 1) {
					int roll = makeRoll(1, 5);
					if (roll < diff) continue;
				}
				
				v.push_back(region);
			}
		}

		for (auto &region : v) {
			visited.insert(region);
		}
	}

	std::vector<ZoneRegion *> list;
	list.reserve(visited.size());
	for (auto &region : visited) {
		list.push_back(region);
	}

	return list;
}

void MapBuilder::SpecializeZones(int continents, int continentAreaFraction) {
	Awrite("Specialize zones");

	int maxArea = (this->w * this->h * continentAreaFraction) / 200;
	int attempts = 0;

	Awrite("Place continent cores");
	std::vector<Zone *> cores;
	attempts = 0;
	while (attempts++ < 1000 && cores.size() != continents) {
		Zone* candidate = this->zones[getrandom(this->zones.size())];

		for (auto &core : cores) {
			if (core == candidate || core->AtBorderWith(candidate)
				|| core->location.Distance(candidate->location) < 8) {
				candidate = NULL;
				break;
			}
		}

		if (candidate != NULL) {
			cores.push_back(candidate);
			attempts = 0;
		}
	}

	int coveredArea = 0;
	std::vector<Zone *> S;
	for (auto &zone : cores) {
		zone->type = ZoneType::CONTINENT;
		coveredArea += zone->regions.size();
		S.push_back(zone);
	}

	Awrite("Grow continents");
	attempts = 0;
	std::vector<Zone *> next;
	while (attempts++ < 10000 && coveredArea <= maxArea && S.size() > 0) {
		int i = getrandom(S.size());
		Zone* zone = S[i];

		if (zone->regions.size() > maxContinentArea) {
			S.erase(S.begin() + i);
			continue;
		}

		next.clear();
		for (auto &nz : zone->neighbors) {
			Zone* nextZone = nz.second;
			if (nextZone->type != ZoneType::UNDECIDED) continue;
			next.push_back(nextZone);
		}

		if (next.size() == 0) {
			S.erase(S.begin() + i);
			continue;
		}

		Zone* target = next[getrandom(next.size())];

		coveredArea += target->regions.size();
		MergeZoneInto(target, zone);
	}

	ClearEmptyZones();

	std::vector<Zone *> oceans;
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		if (zone->regions.size() != 0 && zone->type == ZoneType::UNDECIDED) {
			oceans.push_back(zone);
		}
	}

	Awrite("Add gaps between continents");
	Zone* nonIsland = GetNotIsland();
	while (nonIsland != NULL) {
		Awrite("Starting border cleanup ");
		Zone* otherZone = FindConnectedContinent(nonIsland);
		bool removeFromA;
		bool removeFromB;

		int depthRoll = makeRoll(1, this->gapMax - this->gapMin) + this->gapMin;
		int randomRoll = makeRoll(1, 2);
		int sideRoll = makeRoll(1, 2);

		int sideADepth;
		int sideBDepth;
		if (sideRoll == 1) {
			sideADepth = depthRoll / 2;
			sideBDepth = depthRoll - sideADepth;
		}
		else {
			sideBDepth = depthRoll / 2;
			sideADepth = depthRoll - sideBDepth;
		}

		bool sideARandom = false;
		bool sideBRandom = false;
		if (depthRoll == 1) {
			if (sideADepth == 0) {
				sideADepth = 1;
				sideARandom = true;	
			}
			else {
				sideBDepth = 1;
				sideBRandom = true;	
			}
		}
		else if (depthRoll == 2) {
			sideBRandom = depthRoll == 2 && randomRoll == 1;
			sideARandom = depthRoll == 2 && randomRoll == 2;
		}

		// find regions that are on border
		Zone* strait = CreateZone(ZoneType::STRAIT);
		std::list<ZoneRegion *> list;
		if (sideADepth) {
			auto tmp = FindBorderRegions(nonIsland, otherZone, sideADepth, sideARandom);
			for (auto &r : tmp) {
				list.push_back(r);
			}
		}

		if (sideBDepth) {
			auto tmp = FindBorderRegions(otherZone, nonIsland, sideBDepth, sideBRandom);
			for (auto &r : tmp) {
				list.push_back(r);
			}
		}

		for (auto &r : list) {
			nonIsland->RemoveRegion(r);
			otherZone->RemoveRegion(r);
			strait->AddRegion(r);
		}
		
		SplitZone(strait);

		if (sideADepth > 0) SplitZone(nonIsland);
		if (sideBDepth > 0) SplitZone(otherZone);

		ConnectZones();
		nonIsland = GetNotIsland();
	}

	ClearEmptyZones();

	Awrite("Grow oceans");
	int oceanCores = std::max(1, (int) oceans.size() / 4);
	std::random_shuffle(oceans.begin(), oceans.end());
	oceans.resize(oceanCores);
	for (auto &zone : oceans) {
		zone->type = ZoneType::OCEAN;
	}

	attempts = 0;
	while (oceans.size() > 0) {
		int i = getrandom(oceans.size());
		Zone* ocean = oceans[i];

		std::vector<Zone *> next;
		for (auto &kv : ocean->neighbors) {
			auto zone = kv.second;
			if (zone->type == ZoneType::UNDECIDED) {
				next.push_back(zone);
			}
		}

		if (next.size() > 0) {
			MergeZoneInto(next[getrandom(next.size())], ocean);
		}
		else {
			oceans.erase(oceans.begin() + i);
		}
	}

	ClearEmptyZones();
	ConnectZones();


	Awrite("Cleanup oceans, islands and straits");

	std::stack<Zone *> toCleanUp;
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		int size = zone->regions.size();

		switch (zone->type) {
			case ZoneType::CONTINENT:
			case ZoneType::OCEAN:
				if (size <= 3) toCleanUp.push(zone);
				break;

			case ZoneType::STRAIT:
				toCleanUp.push(zone);
				break;
		}
	}

	while (!toCleanUp.empty()) {
		Zone* src = toCleanUp.top();
		int size = src->regions.size();

		toCleanUp.pop();

		if (size == 0) continue;

		switch (src->type) {
			case ZoneType::CONTINENT: {
				for (auto &kv : src->neighbors) {
					auto n = kv.second;
					if (n->type == ZoneType::CONTINENT) continue;

					// merge into nearest ocean or strait
					MergeZoneInto(src, n);
					break;
				}
				break;
			}

			case ZoneType::OCEAN: {
				if (size > makeRoll(3, 12)) break;

				for (auto &kv : src->neighbors) {
					auto n = kv.second;
					if (n->type != ZoneType::OCEAN) continue;

					// merge into nearest ocean
					MergeZoneInto(src, n);
					break;
				}

				break;
			}

			case ZoneType::STRAIT: {
				int maxStraitSize = 12 + makeRoll(1, 6) - 3;
				if (size > maxStraitSize) continue;

				std::vector<Zone *> otherStraits;
				for (auto &kv : src->neighbors) {
					auto n = kv.second;
					if (n->type != ZoneType::STRAIT) continue;
					if (n->regions.size() > maxStraitSize) continue;

					// merge surrounding straits into src
					otherStraits.push_back(n);
				}

				for (auto &n : otherStraits) {
					if (src->regions.size() > maxStraitSize) break;

					MergeZoneInto(n, src);
				}

				break;
			}
		}
	}

	ClearEmptyZones();

	// make straits surounded only by water to be ZoneType::OCEAN
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		if (zone->type != ZoneType::STRAIT) continue;

		bool skip = false;
		for (auto &n : zone->neighbors) {
			if (n.second->type != ZoneType::STRAIT && n.second->type != ZoneType::OCEAN) {
				skip = true;
				break;
			}
		}

		if (skip) continue;

		zone->type = ZoneType::OCEAN;
	}
}

const int BIOME_ZONES = 4;
const int MAX_BIOMES = 8;
const int BIOMES[BIOME_ZONES][MAX_BIOMES] = {
	// 0: Arctic regions
	{ 4, R_TUNDRA, R_MOUNTAIN, R_FOREST, R_PLAIN, -1, -1, -1 },

	// 1: Colder regions
	{ 5, R_TUNDRA, R_MOUNTAIN, R_FOREST, R_PLAIN, R_SWAMP, -1, -1 },

	// 2: Warmer regions
	{ 7, R_TUNDRA, R_MOUNTAIN, R_FOREST, R_PLAIN, R_SWAMP, R_JUNGLE, R_DESERT },

	// 3: Tropical regions
	{ 5, R_MOUNTAIN, R_PLAIN, R_SWAMP, R_JUNGLE, R_DESERT, -1, -1 }
};

void MapBuilder::AddVolcanoes() {
	Awrite("Adding volcanoes");
	// volcanos will be added anywhere

	int count = makeRoll(1, this->volcanoesMax - this->volcanoesMin) + this->volcanoesMin;
	int distance = (std::min(this->w, this->h / 2) * 2) / count + 2;

	int cols = count / makeRoll(1, 4) + 1;
	int rows = count / cols + 1;
	int dX = this->w / cols;
	int dY = this->h / rows;

	std::vector<ZoneRegion *> candidates;
	std::vector<Coords> volcanoes;
	volcanoes.reserve(cols * rows);

	for (int c = 0; c < cols && volcanoes.size() < count; c++) {
		for (int r = 0; r < rows && volcanoes.size() < count; r++) {
			// if (makeRoll(1, 3) <= 1) continue;

			int attempts = 1000;
			while (attempts-- > 0) {
				int x = c * dX + getrandom(dX);
				int y = r * dY + getrandom(dY);
				if ((x + y) % 2) continue;

				attempts = 0;

				ZoneRegion* seed = this->GetRegion(x, y);
				for (int i = 0; i < volcanoes.size(); i++) {
					int d = volcanoes[i].Distance(seed->location);
					if (d < distance) {
						seed = NULL;
						break;
					}
				}
				if (seed == NULL) continue;

				volcanoes.push_back(seed->location);

				int volcano = makeRoll(1, 3) - 1;
				int mountains = makeRoll(2, 3);

				Zone* zone = this->CreateZone(ZoneType::VOLCANO);
				seed->biome = R_VOLCANO;
				seed->region->type = R_VOLCANO;
				seed->region->wages = AGetName(0, seed->region);

				seed->zone->RemoveRegion(seed);
				zone->AddRegion(seed);

				while (volcano > 0 || mountains > 0) {
					candidates.clear();
					for (auto &kv : zone->regions) {
						auto region = kv.second;
						
						if (region->IsInner()) continue;

						for (int i = 0; i < NDIRS; i++) {
							auto n = region->neighbors[i];
							if (n == NULL) continue;
							if (n->zone == zone) continue;

							candidates.push_back(n);
						}
					}

					int regionSize = zone->regions.size();
					for (int i = 0; regionSize == zone->regions.size(); i++) {
						auto &next = candidates[i % candidates.size()];
						int neigbors = next->CountNeighbors(zone);

						// 2d6
						int roll = makeRoll(2, 6);
						int diff = 12 - neigbors;
						if (regionSize == 1 || candidates.size() == 1) {
							diff = 0;
						}

						// connections: 5, diff: 7, prob: 58%
						// connections: 4, diff: 8, prob: 41%
						// connections: 3, diff: 9, prob: 27%
						// connections: 2, diff: 10, prob: 16%
						// connections: 1, diff: 11, prob: 8%

						if (roll >= diff) {
							if (volcano > 0) {
								next->biome = R_VOLCANO;
								next->region->type = R_VOLCANO;
								next->region->wages = seed->region->wages;

								volcano--;
							}
							else {
								next->biome = R_MOUNTAIN;
								next->region->type = R_MOUNTAIN;
								next->region->wages = seed->region->wages;

								mountains--;
							}

							next->zone->RemoveRegion(next);
							zone->AddRegion(next);
						}
					}
				}
			}
		}
	}

	this->ConnectZones();
}

void MapBuilder::AddLakes() {
	Awrite("Adding lakes");
	// lakes can appear only on the land, not near water and not on the mountains

	int attempts = 1000;
	int count = makeRoll(1, this->lakesMax - this->lakesMin) + this->lakesMin;
	int distance = (std::min(this->w, this->h / 2) * 2) / count;

	std::vector<Coords> lakes;
	lakes.reserve(count);

	while (attempts-- > 0 && count > 0) {
		int x = getrandom(this->w);
		int y = getrandom(this->h);
		if ((x + y) % 2) continue;

		ZoneRegion* lake = this->GetRegion(x, y);
		if (lake->zone->type != ZoneType::CONTINENT) continue;
		if (!lake->IsInner()) continue;
		if (lake->biome == R_VOLCANO || lake->biome == R_MOUNTAIN) continue;

		for (int i = 0; i < lakes.size(); i++) {
			int d = lakes[i].Distance(lake->location);
			if (d < distance) {
				lake = NULL;
				break;
			}
		}

		if (lake == NULL) continue;

		lakes.push_back(lake->location);

		lake->biome = R_LAKE;
		lake->region->type = R_LAKE;
		lake->region->wages = AGetName(0, lake->region);

		count--;
	}
}

void MapBuilder::GrowTerrain() {
	for (auto &kv : this->zones) {
		auto zone = kv.second;
		if (zone->type == ZoneType::OCEAN || zone->type == ZoneType::STRAIT || zone->type == ZoneType::UNDECIDED) {
			Awrite("Grow ocean");
			if (zone->type == ZoneType::UNDECIDED) {
				zone->type = ZoneType::OCEAN;
			}

			for (auto &reg : zone->regions) {
				reg.second->biome = R_OCEAN;
				reg.second->region->type = R_OCEAN;
			}
		}

		if (zone->type == ZoneType::CONTINENT) {
			this->GrowLandInZone(zone);
		}
	}

	this->AddVolcanoes();
	this->AddLakes();
}

void MapBuilder::GrowLandInZone(Zone* zone) {
	Awrite("Growing land in zone");

	if (!zone->CheckZoneIntegerity()) {
		Awrite("Land zone cannot be grown as it have lost integrity");
		exit(1);
	}

	// start with creating provinces
	std::vector<ZoneRegion *> candidates;
	for (auto &kv : zone->regions) {
		candidates.push_back(kv.second);
	}

	// get average province size for this zone
	int provinceSize = makeRoll(2, 4) + 6;
	int provinceCount = zone->regions.size() / provinceSize + 1;

	// absolute size one province cannot exceed
	int maxProvinceSize = zone->regions.size() / 2 + 2;

	// place province seeds so that there are at least 2 free hexes between
	std::vector<ZoneRegion *> S;
	int attempts = 0;
	while (attempts++ < 1000 && S.size() < provinceCount) {
		ZoneRegion* next = candidates[getrandom(candidates.size())];

		for (auto &seed : S) {
			if (seed->location.Distance(next->location) < 3) {
				next = NULL;
				break;
			}
		}
		if (next == NULL) continue;

		S.push_back(next);
		attempts = 0;
	}

	// assign seeds to provinces
	std::vector<Province *> provinces;
	for (auto &seed : S) {
		auto p = zone->CreateProvince(seed, this->h);
		provinces.push_back(p);
	}

	// as each islands must have min 2 biomes, then there must be min 2 provinces
	if (zone->provinces.size() < 2) {
		attempts = 0;
		while (attempts++ < 1000) {
			ZoneRegion* next = candidates[getrandom(candidates.size())];
			if (next->province != NULL) continue;
			
			provinces.push_back(zone->CreateProvince(next, this->h));
			break;
		}
	}

	// grow all provinces in random order
	while (provinces.size() > 0) {
		int i = getrandom(provinces.size());
		auto p = provinces[i];
		int size = p->GetSize();

		if (size >= maxProvinceSize || (size >= provinceSize + getrandom(5) - 2) || !p->Grow()) {
			provinces.erase(provinces.begin() + i);
			continue;
		}
	}

	// find "holes" and assign them to the provinces
	provinces.clear();
	for (auto &kv : zone->regions) {
		auto reg = kv.second;
		if (reg->province == NULL) {
			provinces.push_back(zone->CreateProvince(reg, this->h));
		}
	}

	while (provinces.size() > 0) {
		int i = getrandom(provinces.size());
		auto p = provinces[i];
		int size = p->GetSize();

		if (!p->Grow()) {
			provinces.erase(provinces.begin() + i);
			continue;
		}
	}

	// merge all 1 hex provinces with other provinces
	if (zone->provinces.size() > 2) {
		Province* small;
		do {
			small = NULL;
			for (auto &kv : zone->provinces) {
				auto p = kv.second;
				if (p->GetSize() > 1) continue;

				small = p;
				break;
			}

			if (small == NULL) continue;

			// find neighbor provinces
			auto tmp = small->GetNeighbors();
			Province* target = NULL;
			for (auto x : tmp) {
				if (target == NULL) {
					target = x;
					continue;
				}

				if (target->GetSize() > x->GetSize()) {
					target = x;
				}
			}

			for (auto &kv : small->regions) {
				auto reg = kv.second;
				target->AddRegion(reg);
			}
			zone->RemoveProvince(small);
		}
		while (small != NULL);
	}

	// set biomes for provinces	
	std::unordered_set<int> biomes;
	for (auto &kv : zone->provinces) {
		auto p = kv.second;

		int lat = p->GetLatitude();
		auto latBiomes = BIOMES[lat];
		int biomeCount = latBiomes[0];

		std::vector<int> weights;
		weights.resize(biomeCount);
		int w = 0;
		switch ((int) biomes.size()) {
			// biomes = 0, all weights are equal
			case 0:
				for (int i = 0; i < biomeCount; i++) {
					weights.at(i) = ++w;
				}
				break;

			// biomes = 1, all weights are equal except already present biome where weight will be 0
			case 1:
				for (int i = 0; i < biomeCount; i++) {
					int biome = latBiomes[i + 1];
					weights.at(i) = biomes.find(biome) == biomes.end()
						? ++w
						: 0;
				}
				break;

			// biomes > 2
			// neighbor province biomes: 1 (1/7)
			// already present biomeson the islands: 2 (2/7)
			// missing biome: 4 (4/7)
			default:
				auto nb = p->GetNeighborBiomes();
				for (int i = 0; i < biomeCount; i++) {
					int biome = latBiomes[i + 1];
					if (nb.find(biome) != nb.end()) {
						weights.at(i) = ++w;
					}
					else if (biomes.find(biome) != biomes.end()) {
						weights.at(i) = ++w;
						w += 1;
					}
					else {
						weights.at(i) = ++w;
						w += 3;
					}
				}
				break;
		}

		int roll = makeRoll(1, w);
		for (int i = biomeCount - 1; i >= 0; i--) {
			int diff = weights.at(i);
			if (diff == 0) continue;
			if (roll >= diff) {
				int biome = latBiomes[i + 1];
				p->biome = biome;
				biomes.insert(biome);
				break;
			}
		}
	}

	// set biomes for regions
	for (auto &kv : zone->provinces) {
		auto p = kv.second;

		int name = -1;
		for (auto &rkv : p->regions) {
			auto reg = rkv.second;

			reg->biome = p->biome;
			reg->region->type = p->biome;

			if (name == -1) {
				name = AGetName(0, reg->region);
			}

			reg->region->wages = name;
		}
	}
}

void MapBuilder::SetOceanNames() {
	for (auto &kv : this->zones) {
		auto ocean = kv.second;
		if (ocean->type == ZoneType::OCEAN || ocean->type == ZoneType::STRAIT) {
			int nameIndex = AGetName(0, ocean->regions.begin()->second->region);
			std::string name = AGetNameString(nameIndex);
			name = ocean->type == ZoneType::OCEAN
				? name + " Sea"
				: name + " Strait";

			for (auto &reg : ocean->regions) {
				reg.second->region->SetName(name.c_str());
			}
		}
	}
}