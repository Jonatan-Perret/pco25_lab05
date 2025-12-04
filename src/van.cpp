#include "van.h"

BikingInterface* Van::binkingInterface = nullptr;
std::array<BikeStation*, NB_SITES_TOTAL> Van::stations{};

Van::Van(unsigned int _id)
    : id(_id),
    currentSite(DEPOT_ID)
{}

void Van::run() {
    while (!PcoThread::thisThread()->stopRequested()) {
        loadAtDepot();
        for (unsigned int s = 0; s < NBSITES; ++s) {
            driveTo(s);
            balanceSite(s);
        }
        returnToDepot();
    }
    log("Van s'arrête proprement");
}

void Van::setInterface(BikingInterface* _binkingInterface){
    binkingInterface = _binkingInterface;
}

void Van::setStations(const std::array<BikeStation*, NB_SITES_TOTAL>& _stations) {
    stations = _stations;
}

void Van::log(const QString& msg) const {
    if (binkingInterface) {
        binkingInterface->consoleAppendText(0, msg);
    }
}

void Van::driveTo(unsigned int _dest) {
    if (currentSite == _dest)
        return;

    unsigned int travelTime = randomTravelTimeMs();
    if (binkingInterface) {
        binkingInterface->vanTravel(currentSite, _dest, travelTime);
    }

    currentSite = _dest;
}

void Van::loadAtDepot() {
    driveTo(DEPOT_ID);

    // TODO: implement this method. If possible, load at least 2 bikes
    cargo.clear();
    std::vector<Bike*> loadedBikes = stations[DEPOT_ID]->getBikes(2);
    cargo.insert(cargo.end(), loadedBikes.begin(), loadedBikes.end());

    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}


void Van::balanceSite(unsigned int _site)
{
    // TODO: implement this method. Balance the number of bikes at the given site
    size_t targetBikes = BORNES - 2;
    size_t currentBikes = stations[_site]->nbBikes();
    if (currentBikes > targetBikes && cargo.size() < VAN_CAPACITY) {
        // Take bikes from site
        size_t bikesToTake = currentBikes - targetBikes;
        std::vector<Bike*> takenBikes = stations[_site]->getBikes(bikesToTake);
        cargo.insert(cargo.end(), takenBikes.begin(), takenBikes.end());
    } else if (currentBikes < targetBikes && !cargo.empty()) {
        // Drop bikes to site
        size_t bikesToDrop = targetBikes - currentBikes;
        std::vector<Bike*> bikesToAdd;
        for (size_t i = 0; i < bikesToDrop && !cargo.empty(); ++i) {
            bikesToAdd.push_back(cargo.back());
            cargo.pop_back();
        }
        stations[_site]->addBikes(bikesToAdd);
    }
    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes()); // Keep somewhere for GUI
    }
}

void Van::returnToDepot() {
    driveTo(DEPOT_ID);

    size_t cargoCount = cargo.size();

    // TODO: implement this method. If the van carries bikes, then leave them
    if (!cargo.empty()) {
        std::vector<Bike*> remainingBikes = stations[DEPOT_ID]->addBikes(cargo);
        cargo = remainingBikes; // Update cargo with any bikes that couldn't be added
    }

    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

Bike* Van::takeBikeFromCargo(size_t type) {
    for (size_t i = 0; i < cargo.size(); ++i) {
        if (cargo[i]->bikeType == type) {
            Bike* bike = cargo[i];
            cargo[i] = cargo.back();
            cargo.pop_back();
            return bike;
        }
    }
    return nullptr;
}

