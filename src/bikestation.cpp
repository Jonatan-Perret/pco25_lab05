#include "bikestation.h"

BikeStation::BikeStation(int _capacity) : capacity(_capacity) {}

BikeStation::~BikeStation() {
    ending();
}

void BikeStation::putBike(Bike* _bike){
    mutex.lock();
    while(nbBikes() >= nbSlots()){
        // wait until there's space
        bikeRemoved.wait(&mutex);
    }
    // can add bike
    bikesByType[_bike->bikeType].push_back(_bike);

    bikeAdded.notifyAll();
    mutex.unlock();
}

Bike* BikeStation::getBike(size_t _bikeType) {
    Bike* bike = nullptr;
    mutex.lock();
    while(nbBikes() == 0){ // size_t nbBikes() can't be negative
        // wait until there's a bike
        bikeAdded.wait(&mutex);
    }
    // can get bike
    bike = bikesByType[_bikeType].front();
    bikesByType[_bikeType].pop_front();
    
    bikeRemoved.notifyAll();
    mutex.unlock();
    return bike;
}

std::vector<Bike*> BikeStation::addBikes(std::vector<Bike*> _bikesToAdd) {
    std::vector<Bike*> result; // Can be removed, it's just to avoid a compiler warning
    // TODO: implement this method
    return result;
}

std::vector<Bike*> BikeStation::getBikes(size_t _nbBikes) {
    std::vector<Bike*> result; // Can be removed, it's just to avoid a compiler warning
    // TODO: implement this method
    return result;
}

size_t BikeStation::countBikesOfType(size_t type) const {    
    return bikesByType[type].size();
}

size_t BikeStation::nbBikes() {
    size_t total = 0;
    for(size_t i = 0; i < Bike::nbBikeTypes; i++){
        total += bikesByType[i].size();
    }
    return total;
}

size_t BikeStation::nbSlots() {
    return capacity;
}

void BikeStation::ending() {
   // TODO: implement this method
}
