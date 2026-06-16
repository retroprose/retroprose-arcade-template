// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

/*
         
    

     1 - *
     2 - *    *
     3 - *    *    *
     4 - *    *    *    *
     5 - ***  **
     6 - **   **   **
     7 - ***  **   **
     8 - **   **   **   **
     9 - ***  ***  ***
    10 - ***  ***  **   **
    11 - ***  ***  ***  **
    12 - ***  ***  ***  ***
    13 - **** ***  ***  ***
    14 - **** **** ***  ***
    15 - **** **** ***  ***
    16 - **** **** **** ****
*/

#include <emscripten/bind.h>


class WarpDrive {
public:
    
    // Bit field packing, use 'sizeof' and 'offsetof' for portability
    struct Input {
        uint8_t connected : 1;
        uint8_t left : 1;
        uint8_t right : 1;
        uint8_t primary : 1;
    };

    struct Vec2 {
        Vec2() { }
        Vec2(int16_t _x, int16_t _y) : x(_x), y(_y) { }
        Vec2 operator+(const Vec2& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
        Vec2 operator-(const Vec2& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
        int16_t x;
        int16_t y;
    };

    class Grid {
    public:
        int16_t rows;
        int16_t cols;
        std::vector<uint8_t> grid;

        void clear() {
            std::memset(&(grid[0]), 0, grid.size());
        }

        void resize(int16_t c, int16_t r) {
            rows = r;
            cols = c;
            grid.resize(cols*rows);
        }

        int32_t getIndex(Vec2 p) const {
            return p.y * cols + p.x;
        }

        bool inRange(Vec2 p) const {
            return p.x >= 0 && p.x < cols && p.y >= 0 && p.y < rows;
        }

        Vec2 getPos(int32_t i) const {
            return { (int16_t)(i % cols), (int16_t)(i / cols) };
        }

        size_t size() const {
            return grid.size();
        }

        void decerment(int32_t i) {
            decerment(getPos(i));
        }

        void decerment(Vec2 p) {
            if (inRange(p)) {
                --grid[getIndex(p)];
            }
        }

        void incerment(int32_t i) {
            incerment(getPos(i));
        }

        void incerment(Vec2 p) {
            if (inRange(p)) {
                ++grid[getIndex(p)];
            }
        }

        void set(int32_t i, uint8_t v) {
            set(getPos(i), v);
        }

        void set(Vec2 p, uint8_t v) {
            if (inRange(p)) {
                grid[getIndex(p)] = v;
            }
        }

        uint8_t get(int32_t i) {
            return get(getPos(i));
        }

        uint8_t get(Vec2 p) {
            if (inRange(p)) {
                return grid[getIndex(p)];
            } else {
                return 255;
            }
        }

    };

    struct EntityFlags {
        uint8_t allocated : 1;
        uint8_t active : 1;
        uint8_t dead : 1;
        uint8_t crash : 1;
        uint8_t bounce : 1;
        uint8_t crashKills : 1;
    };

    struct Entity {
        enum Enum : int16_t {
            None = 0,
            ShipTeam1 = 1,
            ShipTeam2 = 2,
            ShipTeam3 = 3,
            ShipTeam4 = 4,
            WarpTeam1 = 5,
            WarpTeam2 = 6,
            WarpTeam3 = 7,
            WarpTeam4 = 8,
            Cell = 9,
            Rock = 10
        };
        static Entity make() { return { 0, Vec2(0, 0), 0, -1, 0, 0, 0, 0, 0, -1, {0,0,0,0} }; }
        int16_t index;
        Vec2 pos;
        int16_t type;
        int16_t slot;
        int16_t turn;
        int16_t direction;
        int16_t offset;
        int16_t speed;
        int16_t trail;
        int16_t ticker;
        EntityFlags flags;
    };

    struct Slot {
        Slot() : entity(0), input({0,0,0,0}), last({0,0,0,0}) { }
        int16_t dock;
        int16_t entity;
        Input input;
        Input last;
    };

    struct Event {
        enum Enum : int16_t {
            None = 0,
            Start,
            Create,
            Destroy
        };
        int16_t id;
        int16_t index;
        Vec2 pos;
        int16_t value;
    };

    uint32_t seed;
    Vec2 docks[16];
    std::vector<Slot> slots;
    std::vector<Entity> entities;
    std::vector<Event> events;
    Grid grid;
    Grid gridMap;
    int16_t teamWin;
    int16_t tileSpeedScale;
    int16_t gridWidth;
    int16_t gridHeight;
    int16_t rockTick;
    int16_t warpTick;
    int16_t startTick;

    // constructor
    WarpDrive() {
        teamWin = 11;
        startTick = 60 * 3;
    }

    int16_t createEntity() {
        int16_t index;
        int16_t head = entities[0].index;
        if (head == 0) {
            index = entities.size();
            entities.push_back(Entity::make());
        } else {
            index = head;
            entities[0].index = entities[index].index;
            entities[index] = Entity::make();
        }
        entities[index].index = index;
        entities[index].flags.allocated = 1;
        return index;
    }

    void destroyEntity(int16_t index) {
        //printf("index, size: %d, %d\n", index, entities.size());
        assert(index < entities.size());
        if (entities[index].flags.allocated == true) {
            entities[index] = Entity::make();
            entities[index].index = entities[0].index;
            entities[0].index = index;
        }
    }

    uint32_t next_rand() {
        uint32_t a = 1103515245;
        uint32_t c = 12345;
        uint32_t m = 2147483648;
        seed = (a * seed + c) % m;
        return seed;
    }

    uint32_t next_rand(int32_t min, int32_t max) {
        return next_rand() % (max - min + 1) + min;
    }

    Vec2 rotate(Vec2 v, int16_t angle) {
        switch (angle) {
            case 0:  return Vec2( v.x,  v.y); break;
            case 1:  return Vec2(-v.y,  v.x); break;
            case 2:  return Vec2(-v.x, -v.y); break;
            case 3:  return Vec2( v.y, -v.x); break;
            default: return Vec2(   0,    0); break;
        };
    }

    void reset() {
        tileSpeedScale = 8;
 
        uint8_t t;
        int16_t i = 0;

        events.clear();

        int16_t dock = 0;

        int8_t dockMapTwo[] =   {0,2,4,6,8,-1};
        int8_t dockMapThree[] = {0,1,2,4,5,6,8,9,10,-1};
        int8_t dockMapFour[] =  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,-1};
        int8_t* dockMap;

        int16_t connectedPlayerCount = 0;
        for (Slot& slot : slots) {
            if (slot.input.connected == true) {
                ++connectedPlayerCount;
            }            
        }

        switch (connectedPlayerCount) {
            case 2:  dockMap = dockMapTwo;   break;
            case 5:  dockMap = dockMapTwo;   break;
            case 6:  dockMap = dockMapThree; break;
            case 7:  dockMap = dockMapThree; break;
            case 8:  dockMap = dockMapFour;  break;
            case 9:  dockMap = dockMapThree; break;
            default: dockMap = dockMapFour;  break;
        }

        entities.clear();
        entities.push_back(Entity::make());
        for (Slot& slot : slots) {
            slot.dock = -1;
            slot.entity = 0;
            if (slot.input.connected == true) {
                int16_t entityIndex = createEntity();
                Entity& e = entities[entityIndex];
                slot.entity = entityIndex;
                slot.dock = dockMap[dock];
                assert(slot.dock != -1);
                e.slot = i;
                e.flags.dead = 0;
                e.pos.x = docks[slot.dock].x;
                e.pos.y = docks[slot.dock].y;
                e.speed = 12;
                e.trail = 64;
                e.type = (slot.dock % 4) + 1;
                e.direction = slot.dock % 4;
                e.flags.crashKills = 1;
                e.flags.active = 1;
                ++dock;
            }
            ++i;
        }

        rockTick = 1;
        warpTick = 1;
    
    }

    bool update() {

        if (startTick > 0) {
            --startTick;
            return false;
        }

        uint8_t b, a;
        int16_t i, oldOffset;
        Vec2 c, oldVec;

        // process events
        for (i = 0; i < events.size(); ++i) {
            switch (events[i].id) {
                case Event::Start: {
                    reset();
                } break;

                case Event::Create: {
                    int16_t entId = createEntity();
                    Entity& ne = entities[entId];
                    ne.offset = 3 * tileSpeedScale;
                    ne.speed = 0;
                    ne.pos = events[i].pos;
                    ne.ticker = events[i].value;
                    ne.type = events[i].index;
                    ne.flags.active = 1;
                    //grid.set(ne.pos.x, ne.pos.y, Grid::Occupied);
                } break;

                case Event::Destroy: {
                    Entity& e = entities[events[i].index];
                    //grid.set(e.pos.x, e.pos.y, Grid::Clear);
                    if (e.slot != -1) {
                        //printf("destroyed ship %d\n", e.slot);
                        slots[e.slot].entity = 0;
                    }
                    destroyEntity(events[i].index);
                } break;

                default: {
                    // unknown event
                } break;
            }
        }
        events.clear();

        --warpTick;
        if (warpTick <= 0) {
            //warpTick = next_rand(200, 300);
            warpTick = 60 * 3;
            int16_t entityIndex = createEntity();
            Entity& e = entities[entityIndex];
            e.flags.dead = 0;
            e.pos.x = next_rand(100, 199);
            e.pos.y = next_rand(100, 199);
            e.speed = 0;
            e.type = Entity::Cell;
            e.offset = tileSpeedScale * 3;
            e.ticker = -1;
            e.turn = 0;
            e.direction = 0;
            e.flags.crashKills = 1;
            e.flags.active = 1;
            //grid.set(e.pos.x, e.pos.y, Grid::Occupied);
        }

        --rockTick;
        if (rockTick == 0) {
            //rockTick = next_rand(10, 50);
            rockTick = 60 * 1;
            int16_t entityIndex = createEntity();
            Entity& e = entities[entityIndex];
            e.flags.dead = 0;
            //e.pos.x = next_rand(100, 199);
            //e.pos.y = next_rand(100, 199);
            e.speed = 6;
            e.type = Entity::Rock;
            e.ticker = -1;
            e.turn = 0;
            e.direction = next_rand(0, 399) / 100;
            switch (e.direction) {
            case 0: e.pos.x = next_rand(0, 49); e.pos.y = next_rand(100, 199); break;
            case 1: e.pos.x = next_rand(100, 199); e.pos.y = next_rand(0, 49); break;
            case 2: e.pos.x = next_rand(137, 299); e.pos.y = next_rand(100, 199); break;
            case 3: e.pos.x = next_rand(100, 199); e.pos.y = next_rand(137, 299); break;
            default: break;
            };
            e.flags.bounce = 1;
            e.flags.active = 1;
            //grid.set(e.pos.x, e.pos.y, Grid::Occupied);
        }

        // clear grid
        grid.clear();
        //printf("dim: %d, %d\n", grid.cols, grid.rows);
        // reset borders
        /*for (i = 0; i < grid.rows; ++i) {
            grid.set(0, i, 1);
            grid.set(grid.cols - 1, i, 1);
        }
        for (i = 0; i < grid.cols; ++i) {
            grid.set(i, 0, 1);
            grid.set(i, grid.rows - 1, 1);
        }*/

        for (Slot& item : slots) {
            if (item.entity != 0) {
                if (item.input.connected == 0) {
                    //printf("player disconnected %d\n", item.entity);
                    entities[item.entity].flags.dead = 1;
                    /*Entity& e = entities[item.entity];
                    int16_t sum = 0;                    
                    sum += grid.get(e.pos + rotate(Vec2(1, 0), e.direction)) & 0x7f;
                    sum += grid.get(e.pos + rotate(Vec2(2, 0), e.direction)) & 0x7f;
                    sum += grid.get(e.pos + rotate(Vec2(3, 0), e.direction)) & 0x7f;
                    sum += gridMap.get(e.pos + rotate(Vec2(1, 0), e.direction));
                    sum += gridMap.get(e.pos + rotate(Vec2(2, 0), e.direction));
                    sum += gridMap.get(e.pos + rotate(Vec2(3, 0), e.direction));
                    //printf("sum: %d\n", sum);
                    if (sum > 0) {
                        int16_t leftDir = (e.direction == 0) ? leftDir = 3 : leftDir = e.direction - 1;
                        int16_t rightDir = (e.direction == 3) ? rightDir = 0 : rightDir = e.direction + 1;
                        int16_t leftSum = 0;
                        int16_t rightSum = 0;
                        leftSum += grid.get(e.pos + rotate(Vec2(1, 0), leftDir)) & 0x7f;
                        leftSum += gridMap.get(e.pos + rotate(Vec2(1, 0), leftDir));                    
                        rightSum += grid.get(e.pos + rotate(Vec2(1, 0), rightDir)) & 0x7f;
                        rightSum += gridMap.get(e.pos + rotate(Vec2(1, 0), rightDir));
                        if (leftSum < rightSum) {
                            e.turn = -1;
                        } else {
                            e.turn = 1;
                        }
                    }*/
                } else {
                    if (item.last.left == 0 && item.input.left == 1) {
                        entities[item.entity].turn = -1;
                    }
                    if (item.last.right == 0 && item.input.right == 1) {
                        entities[item.entity].turn = 1;
                    }
                    if (item.last.primary == 0 && item.input.primary == 1) {
                        // nothing yet
                    }
                }
            }
            item.last = item.input;
        }
	
        for (WarpDrive::Entity& e : entities) {
            if (e.flags.active == 1) {
                if (e.ticker > 0) {
                    --e.ticker;
                }
                if (e.ticker == 0) {
                    e.flags.dead = 1;
                }
            }
        }

        // first pass
        for (WarpDrive::Entity& e : entities) {
            if (e.flags.active == 1) {
                if (e.offset / tileSpeedScale < 3 && (e.offset + e.speed) / tileSpeedScale >= 3) {
                    if (e.turn != 0) {
                        e.direction += e.turn;
                        if (e.direction == -1) {
                            e.direction = 3;
                        }
                        if (e.direction == 4) {
                            e.direction = 0;
                        }
                        e.turn = 0;
                    }
                }
                if (e.type == Entity::Cell) {
                    grid.set(e.pos, grid.get(e.pos) | 0x80);    // set highest flag
                } else {
                    grid.incerment(e.pos);
                    if (e.offset + e.speed >= 8 * tileSpeedScale) {
                        Vec2 v = rotate(Vec2(1, 0), e.direction);
                        v.x += e.pos.x;
                        v.y += e.pos.y;
                        grid.incerment(v);
                    }
                }
            }
        }

        // second pass
        for (WarpDrive::Entity& e : entities) {
            if (e.flags.active == 1 && e.flags.dead == 0) {
                b = grid.get(e.pos);
                a = grid.get(e.pos);
                oldOffset = e.offset;
                oldVec = e.pos;
                e.offset += e.speed;
                if (e.offset >= 8 * tileSpeedScale) {
                    e.offset -= 8 * tileSpeedScale;
                    Vec2 v = rotate(Vec2(1, 0), e.direction);
                    if (e.type <= Entity::ShipTeam4) {
                        events.push_back({Event::Create, (int16_t)(e.type + 4), e.pos, e.trail});
                    }
                    e.pos.x += v.x;
                    e.pos.y += v.y;
                    a = grid.get(e.pos);
                    //printf("a: %d, %d, %d\n", e.pos.x, e.pos.y, a);
                }
                if (e.type == Entity::Cell) {
                    if ((a & 0x7f) > 0) {
                        e.flags.crash = 1;
                    }
                } else {
                    if ((a & 0x7f) > 1 || (b & 0x7f) > 1) {                        
                        e.flags.crash = 1;
                        if (e.flags.bounce == 1) {
                            e.pos = oldVec;
                            e.offset = 63 - oldOffset;
                            switch (e.direction) {
                                case 0: e.direction = 2; break;
                                case 1: e.direction = 3; break;
                                case 2: e.direction = 0; break;
                                case 3: e.direction = 1; break;
                                default: break;
                            }
                        }
                    }
                    if (e.type <= Entity::ShipTeam4) {
                        if ((a & 0x80) == 0x80) {
                            //grid.set(e.pos, grid.get(e.pos) & 0x7f); // unset highest bit
                            e.trail += 8;
                        }
                        if (gridMap.get(e.pos) != 0) {
                            e.flags.crash = 1;
                        }
                    }
                }
            }
        }

        for (WarpDrive::Entity& e : entities) {
            if (e.flags.active == 1) {
                if (e.flags.crash == 1 && e.flags.crashKills == 1) {
                    e.flags.dead = 1;
                }
                if (e.flags.dead == 1) {
                    events.push_back({Event::Destroy, e.index, Vec2(0,0), 0});
                }
            }
        }
 
        int8_t teamCount[] = {0,0,0,0};
        int8_t teamsWithPlayers = 0;
        for (Slot& slot : slots) {
            if (slot.entity != 0) {
                ++teamCount[slot.dock % 4];
            }
        }

        teamWin = 10;
        for (size_t in = 0; in < 4; ++in) {
            if (teamCount[in] > 0) {
                ++teamsWithPlayers;
                teamWin = in;
            }
        }

        //printf("%d %d %d %d %d %d\n", slots.size(), teamsWithPlayers, teamCount[0], teamCount[1], teamCount[2], teamCount[3]);
        if ((slots.size() > 1 && teamsWithPlayers <= 1) || teamsWithPlayers == 0 ) {
            startTick = 60 * 3;
            if (teamsWithPlayers == 0) {
                teamWin = 11;
            }
            events.push_back({WarpDrive::Event::Start, 0, WarpDrive::Vec2(0,0), 0});
        } else {
            teamWin = 10;
        }

        return false;
    }


};






// begin adaptor code

class BindInput {
public:
    BindInput() { }
    WarpDrive::Input& get() { return ptr; }

    bool getConnected() const { return ptr.connected == 1; }
    bool getLeft() const { return ptr.left == 1; }
    bool getRight() const { return ptr.right == 1; }
    bool getPrimary() const { return ptr.primary == 1; }

    void setConnected(bool v) { ptr.connected = (v == true) ? 1 : 0; }    
    void setLeft(bool v) { ptr.left = (v == true) ? 1 : 0; }
    void setRight(bool v) { ptr.right = (v == true) ? 1 : 0; }
    void setPrimary(bool v) { ptr.primary = (v == true) ? 1 : 0; }

private:
    WarpDrive::Input ptr;

};


class BindGame {
public:
    // used to convert input to binary, and binary to input array
    class IOBuffer {
    public:
        IOBuffer(emscripten::val d) : pos(0), data(d) { }

        void write(const char* ptr, size_t size) {
            for (size_t i = 0; i < size; ++i) {
                data.set(pos++, ptr[i]);
            }
        }

        void read(char* ptr, size_t size) {
            for (size_t i = 0; i < size; ++i) {
                ptr[i] = data[pos++].as<uint8_t>();
            }
        }

        size_t pos;
        emscripten::val data;
    };

    struct ObjectDisplay {
        uint32_t color;
        uint8_t ascii;
        uint8_t angle_offset;
    };

    BindGame() {
       
    }

    void setup(emscripten::val config) {
        
        local = config["local"].as<int32_t>();
        game.seed = config["seed"].as<uint32_t>();
        
        playerCount = config["playerCount"].as<int32_t>();
        game.slots.resize(playerCount);
        //game.slots.resize(16);

        emscripten::val docks = config["docks"];
        emscripten::val map = config["map"];

        size_t index = 0;
        for (size_t i = 0; i < 4; ++i) {   
            for (size_t j = 0; j < 4; ++j) {
                game.docks[index] = WarpDrive::Vec2(
                    docks[j]["docks"][i][0].as<int16_t>(),
                    docks[j]["docks"][i][1].as<int16_t>()
                );
                ++index;
            }
        }

        game.gridWidth = 300;
        game.gridHeight = 300;

        game.grid.resize(game.gridWidth, game.gridHeight);
        game.gridMap.resize(game.gridWidth, game.gridHeight);

        size_t len = map["length"].as<size_t>();
        for (size_t i = 0; i < len; ++i) {
            game.gridMap.set(i, map[i].as<uint8_t>());
        }

        game.events.push_back({WarpDrive::Event::Start, 0, WarpDrive::Vec2(0,0), 0});
    }

    bool update() {
        return game.update();        
    }

    void copy() {
        
    }

    void forward() {
        
    }

    void render(emscripten::val v) {
        
        WarpDrive::Vec2 vec;
        float alpha, x, y, loc_a;
        int16_t t, angle;
        uint8_t gt;

        ObjectDisplay type_display[] = {
            {0x000000, 0x00, 0},
            {0xb21030, 0x10, 0},
            {0x49a269, 0x10, 0},
            {0xffa200, 0x10, 0},
            {0xa271ff, 0x10, 0},
            {0xb21030, 0xb0, 0},
            {0x49a269, 0xb0, 0},
            {0xffa200, 0xb0, 0},
            {0xa271ff, 0xb0, 0},
            {0x00ffff, 0x25, 0},
            {0x654321, 0x2a, 0}
        };

        /*for (int16_t j = 0; j < game.grid.rows; ++j) {
            for (int16_t i = 0; i < game.grid.cols; ++i) {
                gt = game.grid.get(i, j);
                if (gt > 0) {
                    v.call<void>("sprite", i * 8 + 4, j * 8 + 4, 0xdb, 0x003f00);
                    //v.call<void>("sprite", i * 8 + 4, j * 8 + 4, gt + 48, 0x003f00);
                }
            }
        }*/

        ObjectDisplay display;
        int16_t entityId = game.slots[local].entity;
        for (WarpDrive::Entity& e : game.entities) {
            if (e.flags.active == 1) {
                vec = game.rotate(WarpDrive::Vec2((e.offset / game.tileSpeedScale) - 3, 0), e.direction);
                if (entityId == e.index) {
                    x = e.pos.x * 8 + vec.x + 4;
                    y = e.pos.y * 8 + vec.y + 4;
                    loc_a = e.direction * 90.0f + 90.0f;
                }
                display = type_display[e.type];
                angle = ((e.direction + display.angle_offset) % 4) * 90.0f;
                v.call<void>("sprite", 
                    e.pos.x * 8 + vec.x + 4, 
                    e.pos.y * 8 + vec.y + 4,
                    display.ascii, 
                    display.color,
                    angle
                );
            }
        }
	
        /*for (int16_t j = 0; j < game.grid.rows; ++j) {
            for (int16_t i = 0; i < game.grid.cols; ++i) {
                gt = game.grid.get(i, j);
                if (gt > 0) {
                    v.call<void>("sprite", i * 8 + 4, j * 8 + 4, gt + 48, 0x00ff00);
                }
            }
        }*/


        v.call<void>("setCamera", x, y, loc_a);

        v.call<void>("setTeam", game.teamWin);

    }

    void getInput(BindInput& in, emscripten::val buffer) {
        WarpDrive::Input& input = in.get();
        IOBuffer os(buffer);
        os.write((char*)(&input), sizeof(input));
    }

    void setInput(emscripten::val v) {
        //GalacticMarauders::Input input;
        //printf("size of input: %d\n", sizeof(input));

        IOBuffer is(v);
        uint32_t frame;        
        
        is.read((char*)(&frame), 4);

        for (size_t i = 0; i < playerCount; ++i) {
            WarpDrive::Input& input = game.slots[i].input;
            is.read((char*)(&input), sizeof(input));
            uint8_t firstByte = ((char*)(&input))[0];
            //printf("first byte: %d\n", (int32_t)firstByte);
            input.connected = firstByte != 0xff;
        }
    }

    void setForwardInput(emscripten::val v) {
        //GalacticMarauders::Input input;
        //printf("size of input: %d\n", sizeof(input));

        /*IOBuffer is(v);
        uint32_t frame;        
        
        is.read((char*)(&frame), 4);

        for (size_t i = 0; i < guess.input.size(); ++i) {
            GalacticMarauders::Input& input = guess.input[i];
            is.read((char*)(&input), sizeof(input));
            uint8_t firstByte = ((char*)(&input))[0];
            //printf("first byte: %d\n", (int32_t)firstByte);
            input.connected = firstByte != 0xff;
        }*/
    }
    
private:
    WarpDrive game;
    int16_t local;
    int16_t playerCount;

};


// Binding code
EMSCRIPTEN_BINDINGS(my_class_example) {
    emscripten::class_<BindInput>("BindInput")
        .constructor()
        .property("connected", &BindInput::getConnected, &BindInput::setConnected)
        .property("left", &BindInput::getLeft, &BindInput::setLeft)
        .property("right", &BindInput::getRight, &BindInput::setRight)
        .property("primary", &BindInput::getPrimary, &BindInput::setPrimary)
        ;

    emscripten::class_<BindGame>("BindGame")
        .constructor()
        .function("update", &BindGame::update)
        .function("copy", &BindGame::copy)
        .function("forward", &BindGame::forward)
        .function("render", &BindGame::render)
        .function("setup", &BindGame::setup)
        .function("setInput", &BindGame::setInput)
        .function("setForwardInput", &BindGame::setForwardInput)
        .function("getInput", &BindGame::getInput)
        ;

}