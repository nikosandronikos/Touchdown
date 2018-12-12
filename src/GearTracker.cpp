#include "GearTracker.h"
#include "GearWidget.h"

#include <vector>
#include <fstream>

#include "XPLMDataAccess.h"
#include "XPLMProcessing.h"

// Warning: This is an expensive call and should not be used regularly.
// It is intended to give information required to initialise other structures.
int currentAircraftNumGear() {
    int gearType[10];
    int nGear;
    XPLMDataRef dataRefGearType = XPLMFindDataRef("sim/aircraft/parts/acf_gear_type");
    XPLMGetDatavi(dataRefGearType, gearType, 0, 10);
    for (nGear = 0; nGear < 10 && gearType[nGear] != 0;) {
        //ctxt->debugFile << gearType[ctxt->nGear] << " ";
        nGear++;
    }
    XPLMUnregisterDataAccessor(dataRefGearType);
    //ctxt->debugFile << std::endl << ctxt->nGear << " gear identified." << std::endl;
    return nGear;
}

GearTracker::GearTracker(int numGear, std::ofstream &debugFile)
:   nGear                {numGear},
    onGround             {true},
    airborneTime         {0.0f},
    lastUpdateTime       {0.0f},
    touchDownTime        {0.0f},
    gearOnGround         {std::vector<bool>(numGear)},
    contactFpm           {std::vector<float>(numGear)},
    contactForce         {std::vector<float>(numGear)},
    contactTime          {std::vector<float>(numGear)},
    bounces              {std::vector<int>(numGear)},
    debugFile            {debugFile},
    dataRefTime          {XPLMFindDataRef("sim/time/total_running_time_sec")},
    dataRefGearOnGround  {XPLMFindDataRef("sim/flightmodel2/gear/on_ground")},
    dataRefFpm           {XPLMFindDataRef("sim/flightmodel/position/vh_ind_fpm")},
    dataRefGearType      {XPLMFindDataRef("sim/aircraft/parts/acf_gear_type")},
    dataRefGearDeflection{XPLMFindDataRef("sim/flightmodel2/gear/tire_vertical_deflection_mtr")},
    dataRefGearForce     {XPLMFindDataRef("sim/flightmodel2/gear/tire_vertical_force_n_mtr")},
    dataRefGearX         {XPLMFindDataRef("sim/aircraft/parts/acf_gear_xnodef")},
    dataRefGearZ         {XPLMFindDataRef("sim/aircraft/parts/acf_gear_znodef")}
{

    onGround = anyGearOnGround();

    std::fill(gearOnGround.begin(), gearOnGround.end(), false);
    std::fill(contactFpm.begin(), contactFpm.end(), 0.0f);
    std::fill(contactForce.begin(), contactForce.end(), 0.0f);
    std::fill(contactTime.begin(), contactTime.end(), 0.0f);
    std::fill(bounces.begin(), bounces.end(), 0);
}

GearTracker::~GearTracker() {
    // FIXME: Implement rule of five.
    XPLMUnregisterDataAccessor(dataRefTime);
    XPLMUnregisterDataAccessor(dataRefGearOnGround);
    XPLMUnregisterDataAccessor(dataRefFpm);
    XPLMUnregisterDataAccessor(dataRefGearForce);
    XPLMUnregisterDataAccessor(dataRefGearDeflection);
    XPLMUnregisterDataAccessor(dataRefGearType);
    XPLMUnregisterDataAccessor(dataRefGearX);
    XPLMUnregisterDataAccessor(dataRefGearZ);
}

std::vector<Point2> GearTracker::getGearPos() {
    float gearX[10], gearZ[10];
    XPLMGetDatavf(dataRefGearX, gearX, 0, nGear);
    XPLMGetDatavf(dataRefGearZ, gearZ, 0, nGear);
    std::vector<Point2> pos(nGear);
    for (int i = 0; i < nGear; i++) {
        pos[i].x = gearX[i];
        pos[i].y = gearZ[i];
    }
    // Gotta love move semantics.
    return pos;
}

void GearTracker::update() {
    float time = XPLMGetDataf(dataRefTime);
    float elapsedTime = time - lastUpdateTime;

    if (anyGearOnGround()) {
        if (onGround == false) touchDownTime = time;
        onGround = true;
        airborneTime = 0.0f;
        int tyreOnGround[10];
        XPLMGetDatavi(dataRefGearOnGround, tyreOnGround, 0, nGear);
        float force[10];
        XPLMGetDatavf(dataRefGearForce, force, 0, nGear);
        float fpm = XPLMGetDataf(dataRefFpm);
        for (int i = 0; i < nGear; i++) {
            if (tyreOnGround[i]) {
                if (!gearOnGround[i] && bounces[i] == 0) {
                    // First contact
                    debugFile << "Gear " << i << " first contact @ " << time << ". " << fpm << " fpm. " << std::endl;
                    contactTime[i] = time;
                }
                gearOnGround[i] = true;

                if (force[i] > contactForce[i]) {
                    // Use the moment of maximum force on the gear to record touchdown data.
                    debugFile << "Gear " << i << " touch down, " << fpm << " fpm. " << force[i] << " force. " << std::endl;
                    contactFpm[i] = fpm;
                    contactForce[i] = force[i];
                }
            } else if (contactFpm[i] != 0.0f) {
                gearOnGround[i] = false;
                bounces[i]++;
                debugFile << "Gear " << i << " bounced! (" << bounces[i] << " total)" << std::endl;
            }
        }
    } else {
        std::fill(gearOnGround.begin(), gearOnGround.end(), false);
        airborneTime += elapsedTime;
        if (onGround && airborneTime > 5.0f) {
            // Must be airborne for 5 seconds before we consider the plane
            // truly off the ground and end the landing event.
            onGround = false;
            std::fill(contactFpm.begin(), contactFpm.end(), 0.0f);
            std::fill(contactForce.begin(), contactForce.end(), 0.0f);
            std::fill(contactTime.begin(), contactTime.end(), 0.0f);
            std::fill(bounces.begin(), bounces.end(), 0);
            debugFile << "Aircraft now airborne for " << airborneTime << " secs." << std::endl;
        }
    }
    lastUpdateTime = time;
}

float GearTracker::relativeTime(int gear) {
    return contactTime[gear] - touchDownTime;
}

bool GearTracker::anyGearOnGround() {
    int gearOnGround[10];
    XPLMGetDatavi(dataRefGearOnGround, gearOnGround, 0, nGear);
    for (int i = 0; i < nGear; i++) if (gearOnGround[i]) return true;
    return false;
}
