#pragma once

#include "GearWidget.h"

#include <vector>
#include <fstream>

#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// Tracks contacts with the ground.
class GearTracker {
public:
    GearTracker(int numGear, std::ofstream &debugFile);
    ~GearTracker();

    bool  getOnGround() { return onGround;};
    float getMaxFpm(int gear) { return contactFpm[gear];};
    float getMaxForce(int gear) { return contactForce[gear];};
    int   getBounces(int gear) { return bounces[gear];};
    int   getNGear() { return nGear;};
    bool  getGearOnGround(int gear) { return gearOnGround[gear];};

    std::vector<Point2> getGearPos();
    float relativeTime(int gear);
    bool anyGearOnGround();
    void update();
private:
    int         nGear;
    bool        onGround;
    float       airborneTime;
    float       lastUpdateTime;
    float       touchDownTime;

    std::vector<bool> gearOnGround;
    // Tracks the hardest fpm touchdown during a landing event
    std::vector<float> contactFpm;
    // Tracks the hardest force, per gear, during a landing event
    std::vector<float> contactForce;
    // Tracks the first time of contact with the ground, per gear.
    std::vector<float> contactTime;
    // Tracks the number of times each gear loses contact with the ground during landing
    std::vector<int> bounces;

    std::ofstream    &debugFile;

    XPLMDataRef dataRefTime;
    XPLMDataRef dataRefGearOnGround;
    XPLMDataRef dataRefFpm;
    XPLMDataRef dataRefGearForce;
    XPLMDataRef dataRefGearDeflection;
    XPLMDataRef dataRefGearType;
    XPLMDataRef dataRefGearX;
    XPLMDataRef dataRefGearZ;
};

int currentAircraftNumGear();


