#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace banner {
constexpr int Side=8;
struct Point {int x=0,y=0;bool operator==(Point b)const{return x==b.x&&y==b.y;}bool operator!=(Point b)const{return !(*this==b);}};
enum class Tile {Ground,Grass,TallGrass,Mountain,Water,Building};
class Random {
    std::mt19937 engine;
public:
    explicit Random(uint32_t seed):engine(seed){}
    uint32_t Below(uint32_t bound);
    template<class T> void Shuffle(std::vector<T>& a){for(size_t n=a.size();n>1;--n)std::swap(a[n-1],a[Below(uint32_t(n))]);}
};
// Mechanism reconstruction, not the original executable's PRNG sequence.
size_t WeightedPick(const std::vector<uint32_t>& weights,Random& rng);
std::optional<std::array<Point,2>> SelectDefendedPair(std::vector<Point> zone,Random& rng);
// Original script's greedy selection semantics (without Board side effects).
std::optional<std::array<Point,2>> SelectDefendedPairGreedy(std::vector<Point> zone,Random& rng);
struct BuildingCandidate {Point p;int openFlyerNeighbors=0;};
std::vector<Point> PreferOpenBuildings(const std::vector<BuildingCandidate>& candidates);
struct SpawnOption {int kind=0;bool upgraded=false;int living=0,cap=3;};
std::vector<SpawnOption> FilterSpawnCaps(const std::vector<SpawnOption>& choices,int livingUpgrades,int upgradeCap);

struct Enemy {Point p;int direction=-1;};
struct Map {
    std::array<Tile,64> tiles{};
    std::array<Point,3> players{};
    std::array<Enemy,3> enemies{};
    std::array<Point,2> objectives{};
    uint32_t seed=0;
    int templateId=0;
};
struct Step {int player=-1;Point destination;int enemy=-1;};
struct Proof {bool found=false;bool exhausted=false;size_t nodes=0;std::vector<Step> steps;};
struct Result {std::optional<Map> map;Proof opening;int attempts=0;std::string reason;};
bool Inside(Point p);
bool Walkable(Tile t);
std::vector<std::string> Validate(const Map& map);
// Reference combat only: 3 shooters, move <=3 then kill first enemy along a cardinal ray.
// Enemies stand still with locked directions; each surviving ray hitting a building loses 1 grid.
Proof ProveOpening(const Map& map,size_t nodeBudget=4000);
bool ReplayProof(const Map& map,const Proof& proof);
int OpeningGridDamage(const Map& map);
Result Generate(uint32_t seed,int maxAttempts=64,const std::string& tag="any");
std::string Describe(const Map& map,const Proof& proof);
}
