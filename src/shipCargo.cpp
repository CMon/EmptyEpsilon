#include "shipCargo.h"
#include "cargo.h"
#include "spaceObjects/spaceship.h"
#include "spaceObjects/playerSpaceship.h"
#include "gameGlobalInfo.h"
#include "tween.h"

REGISTER_MULTIPLAYER_CLASS(ShipCargo, "ShipCargo");

ShipCargo::ShipCargo() : Cargo("ShipCargo")
{
    registerMemberReplication(&callsign);
    registerMemberReplication(&template_name);
    registerMemberReplication(&hull_strength);
    registerMemberReplication(&has_reactor);
    for(int n=0; n<SYS_COUNT; n++) {
        registerMemberReplication(&systems_health[n]);
    }
}

ShipCargo::ShipCargo(P<ShipTemplate> ship_template) : ShipCargo()
{
    template_name = ship_template->getName();
    callsign = "DRN-" + gameGlobalInfo->getNextShipCallsign();
    setEnergy(ship_template->energy_storage_amount);
    hull_strength = ship_template->hull;
    has_reactor = ship_template->has_reactor;
    for(int n=0; n<SYS_COUNT; n++) {
        systems_health[n] = 1;
    }
    for(int n=0; n < MW_Count; n++)
    {
//        setWeaponStorage(EMissileWeapons(n), ship_template->weapon_storage[n]);
        setWeaponStorage(EMissileWeapons(n), 0);
        setWeaponStorageMax(EMissileWeapons(n), ship_template->weapon_storage[n]);
    }
}

ShipCargo::ShipCargo(P<SpaceShip> ship) : ShipCargo()
{
    template_name = ship->getTypeName();
    callsign = ship->getCallSign();
    setEnergy(ship->getEnergy());
    hull_strength = ship->getHull();
    has_reactor = ship->has_reactor;
    float totalHeat = 0;
    for(unsigned int n=0; n<SYS_COUNT; n++)
        totalHeat += ship->getSystemHeat(ESystem(n));
    setHeat(totalHeat);
    hull_strength = ship->getHull();
    for(int n=0; n<SYS_COUNT; n++) {
        systems_health[n] = ship->systems[n].health;
    }
    for(int n=0; n < MW_Count; n++)
    {
        setWeaponStorage(EMissileWeapons(n), ship->weapon_storage[n]);
        setWeaponStorageMax(EMissileWeapons(n), ship->weapon_storage_max[n]);
    }
}

P<ModelData> ShipCargo::getModel()
{
    P<ShipTemplate> ship_template = ShipTemplate::getTemplate(template_name);
    if (ship_template)
        return ship_template->model_data;
    else
        return nullptr;
}

float ShipCargo::getHealth()
{
    const float maxHull = getTemplate()->hull;
    float health = hull_strength;
    for(int n=0; n<SYS_COUNT; n++) {
        health += Tween<float>::linear(systems_health[n], -1, 1, 0, maxHull);
    }
    return health;
}

void ShipCargo::addHealth(float amount)
{
    const float maxHull = getTemplate()->hull;
    const float normAmount = amount / getMaxHealth();
    for(int n=0; n<SYS_COUNT; n++) {
        systems_health[n] = std::min(1.0f, systems_health[n] + (2 * normAmount));
    }
    hull_strength = std::min(maxHull, hull_strength + (maxHull * normAmount));
}

bool ShipCargo::onLaunch(Dock &source)
{
    if (game_server)
    {
        P<PlayerSpaceship> ship = new PlayerSpaceship();
        if (ship)
        {
            ship->setFactionId(source.getFactionId());
            ship->setTemplate(template_name);
            ship->setCallSign(callsign);
            ship->setEnergyLevel(getEnergy());
            ship->setPosition(source.getLaunchPosition(ship->getRadius()));
            ship->setRotation(source.getLaunchRotation());
            ship->setHull(hull_strength);
            ship->has_reactor = has_reactor;
            ship->impulse_request = -0.5;
            int systemsCount = 0;
            for (unsigned int n = 0; n < SYS_COUNT; n++){
                if (ship->hasSystem(ESystem(n)))
                    systemsCount++;
                ship->systems[n].health = systems_health[n];
            }
            for (unsigned int n = 0; n < SYS_COUNT; n++)
                if (ship->hasSystem(ESystem(n)))
                    ship->addHeat(ESystem(n), getHeat() / systemsCount);
            for(int n = 0; n < MW_Count; n++)
            {
                ship->weapon_storage[n] = getWeaponStorage(EMissileWeapons(n));
                ship->weapon_storage_max[n] = getWeaponStorageMax(EMissileWeapons(n));
            }
            return true;
        }
    }
    return false;
}

ShipCargo::Entries ShipCargo::getEntries()
{
//    ShipCargo::Entries result = Cargo::getEntries();
    ShipCargo::Entries result;
    P<ShipTemplate> ship_template = ShipTemplate::getTemplate(template_name);
    if (ship_template)
    {
        result.push_back(std::make_tuple("gui/icons/hull", "Carlingue", string(int(100 * hull_strength / ship_template->hull)) + "%"));
    }
    result.push_back(std::make_tuple("", "ID", callsign));
    result.push_back(std::make_tuple("", "type", template_name));

    if (has_reactor)
        result.push_back(std::make_tuple("", "Reacteur ?", "Oui"));
    else
        result.push_back(std::make_tuple("", "Reacteur ?", "Non"));

    float velocity = ship_template->impulse_speed / 1000 * 60;
    result.push_back(std::make_tuple("", "Vitesse", string(velocity, 1) + DISTANCE_UNIT_1K + "/min"));

    if (ship_template->weapon_tube_count > 0)
        result.push_back(std::make_tuple("", "Tubes a missiles", ship_template->weapon_tube_count));

    return result;
}
