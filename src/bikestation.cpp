#include "bikestation.h"
#include <pcosynchro/pcologger.h>

BikeStation::BikeStation(int _capacity) : capacity(_capacity)
{
    PcoLogger::setVerbosity(1);
    shouldEnd = false;
}

BikeStation::~BikeStation()
{
    ending();
}

void BikeStation::putBike(Bike *_bike)
{
    mutex.lock();
    while (nbBikes() >= nbSlots() && !shouldEnd)
    {
        // wait until there's space
        bikeRemoved[_bike->bikeType].wait(&mutex);
    }

    if (shouldEnd)
    {
        mutex.unlock();
        return;
    }

    // can add bike
    bikesByType[_bike->bikeType].push_back(_bike);

    bikeAdded[_bike->bikeType].notifyOne();
    mutex.unlock();
}

Bike *BikeStation::getBike(size_t _bikeType)
{
    Bike *bike = nullptr;
    mutex.lock();
    while (bikesByType[_bikeType].empty() && !shouldEnd)
    {
        // wait until there's a bike of the requested type
        bikeAdded[_bikeType].wait(&mutex);
    }

    if (shouldEnd)
    {
        mutex.unlock();
        return nullptr;
    }

    // can get bike
    bike = bikesByType[_bikeType].front();
    bikesByType[_bikeType].pop_front();

    bikeRemoved[_bikeType].notifyOne();
    mutex.unlock();
    return bike;
}

std::vector<Bike *> BikeStation::addBikes(std::vector<Bike *> _bikesToAdd)
{
    std::vector<Bike *> result;
    // TODO refactor with condition variables to wait if no slots are available
    mutex.lock();
    for (Bike *bike : _bikesToAdd)
    {
        if (nbBikes() < nbSlots())
        {
            // can add bike
            bikesByType[bike->bikeType].push_back(bike);
            bikeAdded[bike->bikeType].notifyOne();
        }
        else
        {
            // cannot add bike, return it
            result.push_back(bike);
        }
    }
    mutex.unlock();
    return result;
}

std::vector<Bike *> BikeStation::getBikes(size_t _nbBikes)
{

    std::vector<Bike *> result;
    // TODO refactor with condition variables to wait if no bikes are available
    mutex.lock();
    for (size_t type = 0; type < Bike::nbBikeTypes; type++)
    {
        while (result.size() < _nbBikes && !bikesByType[type].empty())
        {
            // can get bike
            Bike *bike = bikesByType[type].front();
            bikesByType[type].pop_front();
            result.push_back(bike);
            bikeRemoved[type].notifyOne();
        }
        if (result.size() >= _nbBikes)
        {
            break;
        }
    }
    mutex.unlock();
    return result;
}

size_t BikeStation::countBikesOfType(size_t type) const
{
    return bikesByType[type].size();
}

size_t BikeStation::nbBikes()
{
    size_t total = 0;
    for (size_t i = 0; i < Bike::nbBikeTypes; i++)
    {
        total += bikesByType[i].size();
    }
    return total;
}

size_t BikeStation::nbSlots()
{
    return capacity;
}

void BikeStation::ending()
{
    mutex.lock();
    shouldEnd = true;
    // Wake up all waiting threads

    for (size_t i = 0; i < Bike::nbBikeTypes; i++)
    {
        bikeAdded[i].notifyAll();
        bikeRemoved[i].notifyAll();
    }

    mutex.unlock();
}
