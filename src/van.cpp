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

    // TODO: implement this method. If possible, load at least 2 bikes
    cargo.clear();
    log(QString("Charge des vélos au dépôt"));
    std::vector<Bike *> loadedBikes = stations[DEPOT_ID]->getBikes(VAN_CAPACITY);
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
    /*
    Boucle infinie
        1. Mettre a = min(2, D) vélos dans la camionnette.
        2. Pour i = 1 à S faire
        2a. Si Vi > B−2 alors
        Prendre c = min Vi−(B−2), 4−a vélos du site i et les mettre dans la camionnette.
        a ←a + c.
        2b. Si Vi < B−2 alors
        Calculer c = min (B−2)−Vi, a (nombre de vélos à déposer au site i).
        Initialiser cdéposés ←0.
        Pour chaque type de vélo t
        Si le site i ne contient aucun vélo de type t
        et la camionnette contient au moins un vélo de type t alors
        Déplacer un vélo de type t de la camionnette vers le site i.
        a ←a−1, Vi ←Vi + 1, cdéposés ←cdéposés + 1.
        Si cdéposés= c alors sortir de la boucle sur les types.
        Tant que cdéposés < c et a > 0 faire
        Déplacer un vélo quelconque de la camionnette vers le site i.
        a ←a−1, Vi ←Vi + 1, cdéposés ←cdéposés + 1.
        3. Vider la camionnette au dépôt : D ←D + a, a ←0.
        4. Faire une pause.
    Fin de la boucle
        */
    size_t currentBikeCount = stations[_site]->nbBikes();
    size_t threshold = BORNES - 2;
    size_t cargoSpace = VAN_CAPACITY - cargo.size();

    log(QString("Van at site %1: %2 bikes (target: %3), cargo: %4")
            .arg(_site)
            .arg(currentBikeCount)
            .arg(threshold)
            .arg(cargo.size()));

    // Site has too many bikes - take some
    if (currentBikeCount > threshold && cargoSpace > 0)
    {
        size_t toTake = std::min(currentBikeCount - threshold, cargoSpace);
        std::vector<Bike *> taken = stations[_site]->getBikes(toTake);
        cargo.insert(cargo.end(), taken.begin(), taken.end());

        log(QString("Van takes %1 bike(s) from site %2.")
                .arg(taken.size())
                .arg(_site));

        if (binkingInterface)
            binkingInterface->setBikes(_site, stations[_site]->nbBikes());
    }
    // Site has too few bikes - deposit some
    else if (currentBikeCount < threshold && !cargo.empty())
    {
        size_t toDeposit = std::min(threshold - currentBikeCount, cargo.size());
        size_t deposited = 0;

        // First, deposit missing bike types
        for (size_t type = 0; type < Bike::nbBikeTypes && deposited < toDeposit; ++type)
        {
            if (stations[_site]->countBikesOfType(type) == 0)
            {
                Bike *bike = takeBikeFromCargo(type);
                if (bike)
                {
                    stations[_site]->putBike(bike);
                    deposited++;
                    log(QString("Van deposits missing type %1 at site %2.")
                            .arg(type)
                            .arg(_site));
                }
            }
        }

        // Then, deposit remaining bikes as needed
        while (deposited < toDeposit && !cargo.empty())
        {
            Bike *bike = cargo.back();
            cargo.pop_back();
            stations[_site]->putBike(bike);
            deposited++;
            log(QString("Van deposits bike type %1 at site %2.")
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

    size_t cargoCount = cargo.size();

    // TODO: implement this method. If the van carries bikes, then leave them
    if (!cargo.empty())
    {
        log(QString("Retourne au dépôt avec %1 vélos").arg(cargoCount));
        std::vector<Bike *> remainingBikes = stations[DEPOT_ID]->addBikes(cargo);
        cargo = remainingBikes; // Update cargo with any bikes that couldn't be added
        log(QString("Déchargé %1 vélos au dépôt (%2 non déchargés)")
                .arg(cargoCount - remainingBikes.size())
                .arg(remainingBikes.size()));
    }
    else
    {
        log(QString("Retourne au dépôt (cargo vide)"));
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
