/*
    * van.cpp
    * Author: Jonatan Perret and Adrien Marcuard
*/

#include "van.h"

BikingInterface *Van::binkingInterface = nullptr;
std::array<BikeStation *, NB_SITES_TOTAL> Van::stations{};

Van::Van(unsigned int _id)
    : id(_id),
      currentSite(DEPOT_ID)
{
}

void Van::run()
{
    while (!PcoThread::thisThread()->stopRequested())
    {
        // wait for some time before starting next round
        PcoThread::thisThread()->usleep(VAN_DEPOT_WAITIME);
        loadAtDepot();
        for (unsigned int s = 0; s < NBSITES; ++s)
        {
            driveTo(s);
            balanceSite(s);
        }
        returnToDepot();
    }
    log("Van s'arrête proprement");
}

void Van::setInterface(BikingInterface *_binkingInterface)
{
    binkingInterface = _binkingInterface;
}

void Van::setStations(const std::array<BikeStation *, NB_SITES_TOTAL> &_stations)
{
    stations = _stations;
}

void Van::log(const QString &msg) const
{
    if (binkingInterface)
    {
        binkingInterface->consoleAppendText(0, msg);
    }
}

void Van::driveTo(unsigned int _dest)
{
    if (currentSite == _dest)
        return;

    log(QString("Conduit du site %1 au site %2 (cargo: %3 vélos)")
            .arg(currentSite)
            .arg(_dest)
            .arg(cargo.size()));
    unsigned int travelTime = randomTravelTimeMs();
    if (binkingInterface)
    {
        binkingInterface->vanTravel(currentSite, _dest, travelTime);
    }

    currentSite = _dest;
}

void Van::loadAtDepot()
{
    driveTo(DEPOT_ID);

    cargo.clear();
    log(QString("Charge des vélos au dépôt"));
    
    // Charger a = min(2, D) vélos où D est le nombre de vélos au dépôt
    size_t depotBikes = stations[DEPOT_ID]->nbBikes();
    size_t bikesToLoad = std::min(size_t(2), depotBikes);
    std::vector<Bike *> loadedBikes = stations[DEPOT_ID]->getBikes(bikesToLoad);
    cargo.insert(cargo.end(), loadedBikes.begin(), loadedBikes.end());
    
    log(QString("Chargé %1 vélos (dépôt: %2 vélos restants)")
            .arg(loadedBikes.size())
            .arg(stations[DEPOT_ID]->nbBikes()));

    if (binkingInterface)
    {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

void Van::balanceSite(unsigned int _site)
{
    size_t Vi = stations[_site]->nbBikes();  // Number of bikes at site i
    size_t threshold = BORNES - 2;           // B-2 (target threshold)
    size_t a = cargo.size();                 // Number of bikes in the van

    log(QString("Van at site %1: %2 bikes (threshold: %3), cargo: %4")
            .arg(_site)
            .arg(Vi)
            .arg(threshold)
            .arg(a));

    // 2a. If Vi > B-2 then take bikes
    if (Vi > threshold)
    {
        // c = min(Vi-(B-2), 4-a) bikes to take
        size_t cargoSpace = VAN_CAPACITY - a;
        size_t c = std::min(Vi - threshold, cargoSpace);
        
        if (c > 0)
        {
            std::vector<Bike *> taken = stations[_site]->getBikes(c);
            cargo.insert(cargo.end(), taken.begin(), taken.end());
            
            log(QString("Takes %1 bike(s) from site %2 (surplus)")
                    .arg(taken.size())
                    .arg(_site));

            if (binkingInterface)
                binkingInterface->setBikes(_site, stations[_site]->nbBikes());
        }
    }
    // 2b. If Vi < B-2 then deposit bikes
    else if (Vi < threshold && a > 0)
    {
        // c = min((B-2)-Vi, a) number of bikes to deposit
        size_t c = std::min(threshold - Vi, a);
        size_t deposited = 0;

        // First pass: deposit bikes of completely missing types
        for (size_t type = 0; type < Bike::nbBikeTypes && deposited < c; ++type)
        {
            // If site i contains no bikes of type t
            // and the van contains at least one bike of type t
            if (stations[_site]->countBikesOfType(type) == 0)
            {
                Bike *bike = takeBikeFromCargo(type);
                if (bike)
                {
                    stations[_site]->putBike(bike);
                    deposited++;
                    log(QString("Deposits bike type %1 (missing) at site %2")
                            .arg(type)
                            .arg(_site));
                }
            }
        }

        // While deposited < c and cargo not empty, deposit any bikes
        while (deposited < c && !cargo.empty())
        {
            Bike *bike = cargo.back();
            cargo.pop_back();
            stations[_site]->putBike(bike);
            deposited++;
            log(QString("Deposits bike type %1 at site %2")
                    .arg(bike->bikeType)
                    .arg(_site));
        }

        if (binkingInterface)
            binkingInterface->setBikes(_site, stations[_site]->nbBikes());
    }
}

void Van::returnToDepot()
{
    driveTo(DEPOT_ID);

    size_t a = cargo.size();

    // 3. Vider la camionnette au dépôt : D ← D + a, a ← 0
    if (a > 0)
    {
        log(QString("Retourne au dépôt avec %1 vélos").arg(a));
        std::vector<Bike *> remainingBikes = stations[DEPOT_ID]->addBikes(cargo);
        cargo = remainingBikes;
        
        if (remainingBikes.empty())
        {
            log(QString("Déchargé %1 vélos au dépôt (a=0)").arg(a));
        }
        else
        {
            log(QString("Déchargé %1 vélos au dépôt (%2 non déchargés - dépôt plein)")
                    .arg(a - remainingBikes.size())
                    .arg(remainingBikes.size()));
        }
    }
    else
    {
        log(QString("Retourne au dépôt (cargo vide, a=0)"));
    }

    if (binkingInterface)
    {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

Bike *Van::takeBikeFromCargo(size_t type)
{
    for (size_t i = 0; i < cargo.size(); ++i)
    {
        if (cargo[i]->bikeType == type)
        {
            Bike *bike = cargo[i];
            cargo[i] = cargo.back();
            cargo.pop_back();
            return bike;
        }
    }
    return nullptr;
}
