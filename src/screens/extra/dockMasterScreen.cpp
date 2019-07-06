#include "dockMasterScreen.h"

#include "playerInfo.h"
#include "spaceObjects/shipTemplateBasedObject.h"
#include "spaceObjects/playerSpaceship.h"
#include "screenComponents/shipsLogControl.h"
#include "screenComponents/customShipFunctions.h"

#include "gui/gui2_listbox.h"
#include "gui/gui2_autolayout.h"
#include "gui/gui2_element.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_label.h"
#include "gui/gui2_keyvaluedisplay.h"
#include "gui/gui2_slider.h"
#include "gui/gui2_progressbar.h"
#include "gui/gui2_button.h"
#include "gui/gui2_selector.h"
#include "screenComponents/powerDamageIndicator.h"

#include "screenComponents/rotatingModelView.h"

const int ROW_SIZE = 4;
const int ROW_HEIGHT = 200;
const int BOX_WIDTH = 290;
const int COLUMN_WIDTH = 400;

DockMasterScreen::DockMasterScreen(GuiContainer *owner)
    : GuiOverlay(owner, "DOCK_MASTER_SCREEN", colorConfig.background)
{
    GuiOverlay *background_crosses = new GuiOverlay(this, "BACKGROUND_CROSSES", sf::Color::White);
    background_crosses->setTextureTiled("gui/BackgroundCrosses");

    GuiAutoLayout *rootLayout = new GuiAutoLayout(this, "ROOT_LAYOUT", GuiAutoLayout::LayoutHorizontalLeftToRight);
    rootLayout->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    docks = new GuiListbox(rootLayout, "DOCKS_LIST", [this](int index_dock, string value) {
        selectDock(index_dock);
    });
    docks->setMargins(20, 20, 20, 20)->setSize(COLUMN_WIDTH / 2, GuiElement::GuiSizeMax);
    docks->setPosition(0, 0, ATopLeft);

    // the index in the button list is assumed to equal the index of the dock
    for (int n = 0; n < 10; n++)
        docks->addEntry("dock-"+string(n+1), "Vide");

    bays = new GuiListbox(rootLayout, "DOCKS_LIST", [this](int index_bay, string value) {
        if (index_bay < 3)
            selectBay(index_bay);
    });
    bays->setMargins(20, 20, 20, 20)->setSize(COLUMN_WIDTH / 2, GuiElement::GuiSizeMax);
    bays->setPosition(0, 0, ATopRight);

    // the index in the button list is assumed to equal the index of the dock
    for (int n = 0; n < 3; n++)
        bays->addEntry("Baie-"+string(n+1), "Vide");
    bays->addEntry("space", " ");
    bays->addEntry("energy", "Energie");
    bays->addEntry("weapons", "Missile");
    bays->addEntry("repair", "Reparation");

    mainPanel = new GuiAutoLayout(rootLayout, "TOP_PANEL", GuiAutoLayout::LayoutHorizontalRows);
    mainPanel->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    mainPanel->setPosition(0, 0, ATopCenter);
    mainPanel->setMargins(20, 20, 20, 20);

    topPanel = new GuiAutoLayout(mainPanel, "TOP_PANEL", GuiAutoLayout::LayoutVerticalTopToBottom);
    topPanel->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax / 2.0);
    topPanel->setPosition(0, 0, ATopRight);
//    mainPanel->setMargins(20, 20, 20, 20);

    // Dock actions
    (new GuiLabel(topPanel, "TITLE", "Transfert des Vaisseaux", 30))
        ->addBackground()
        ->setAlignment(ACenter)
        ->setPosition(0, 0, ABottomCenter)
        ->setSize(GuiElement::GuiSizeMax, 50);

    layout_move = new GuiAutoLayout(topPanel, "ACTION_MOVE", GuiAutoLayout::LayoutVerticalColumns);
    layout_move->setSize(GuiElement::GuiSizeMax, 50)->setPosition(0, 50, ATopCenter);

    action_move_button = new GuiButton(layout_move, "MOVE_BUTTON", "Transferer", [this]() {
        if (my_spaceship)
            if (my_spaceship->getSystemEffectiveness(SYS_Docks) > 0)
            {
                if (selected_ship)
                {
                    for (int n = 0; n < 3; n++)
                        if (bays->getEntryName(n) == selected_ship->callsign)
                            bays->setEntryName(n,"Baie-"+string(n+1));
                    selected_ship->id_dock = index_bay + 1;
                    bays->setEntryName(index_bay,selected_ship->callsign);
                    my_spaceship->addToShipLog("Transfert du vaisseau " + selected_ship->callsign + " Vers la baie d'amarrage " + string(index_bay+1),colorConfig.log_generic,"docks");
                    return;
                }
//                PVector<SpaceObject> list_objs = my_spaceship->getObjectsInRange(5000);
//                foreach(SpaceObject, obj, list_objs)
//                {
//                    if (obj->callsign == docks->getEntryName(index_dock))
//                    {
//                        // Remove previous bay record
//                        for (int n = 0; n < 3; n++)
//                            if (bays->getEntryName(n) == obj->callsign)
//                                bays->setEntryName(n,"Baie-"+string(n+1));
//                        obj->id_dock = index_bay + 1;
//                        bays->setEntryName(index_bay,obj->callsign);
//                        my_spaceship->addToShipLog("Transfert du vaisseau " + obj->callsign + " Vers la baie d'amarrage " + string(index_bay+1),colorConfig.log_generic,"docks");
//                        return;
//                    }
//                }
            }
    });
    action_move_button->setSize(COLUMN_WIDTH, 40);

    action_empty_button = new GuiButton(layout_move, "EMPTY_BUTTON", "Liberer", [this]() {
        if (my_spaceship)
            if (my_spaceship->getSystemEffectiveness(SYS_Docks) > 0)
            {
                bays->setEntryName(index_bay,"Baie-"+string(index_bay+1));
            }
    });
    action_empty_button->setSize(COLUMN_WIDTH, 40);
    (new GuiPowerDamageIndicator(action_move_button, "DOCKS_DPI", SYS_Docks, ATopCenter, my_spaceship))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    droneTitle = new GuiLabel(topPanel, "DRONE_TITLE", "Drone x", 30);
    droneTitle->addBackground()
        ->setAlignment(ACenter)
        ->setPosition(0, 0, ATopCenter)
        ->setSize(GuiElement::GuiSizeMax, 50);

    (new GuiLabel(topPanel, "SPACE", " ", 30))->setSize(GuiElement::GuiSizeMax, 50);

    action_weapons = new GuiAutoLayout(topPanel, "ACTION_WEAPONS", GuiAutoLayout::LayoutVerticalColumns);
//    action_weapons = new GuiAutoLayout(topPanel, "ACTION_WEAPONS", GuiAutoLayout::LayoutVerticalTopToBottom);
    action_weapons->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 50, ATopCenter);
    (new GuiLabel(action_weapons, "ACTION_WEAPONS_LABEL", "Transfert missiles :", 30))->setAlignment(ACenterRight)->setMargins(10, 10, 10, 10);

    table_weapons = new GuiAutoLayout(action_weapons, "TABLE_WEAPONS", GuiAutoLayout::LayoutVerticalTopToBottom);
    table_weapons->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    weapons_layout_label = new GuiAutoLayout(table_weapons, "WEAPONS_LAYOUT_LABEL", GuiAutoLayout::LayoutVerticalColumns);
    weapons_layout_label -> setSize(GuiElement::GuiSizeMax, 40);
    (new GuiLabel(weapons_layout_label, "", "Missile", 20));
    (new GuiLabel(weapons_layout_label, "", "Station", 20));
    (new GuiLabel(weapons_layout_label, "", "Vaisseau", 20));
    (new GuiLabel(weapons_layout_label, "", " ", 20));
    (new GuiLabel(weapons_layout_label, "", " ", 20));

    for(int n=0; n<MW_Count; n++)
    {
//        weapons_layout[n] = new GuiElement(table_weapons, "WEAPONS_LAYOUT");
        weapons_layout[n] = new GuiAutoLayout(table_weapons, "WEAPONS_LAYOUT", GuiAutoLayout::LayoutVerticalColumns);
        weapons_layout[n]->setSize(GuiElement::GuiSizeMax, 40);

        (new GuiLabel(weapons_layout[n], "", getMissileWeaponName(EMissileWeapons(n)), 20))->setSize(75, 30);

        weapons_stock_ship[n] = new GuiLabel(weapons_layout[n],"","0/20",20);
        weapons_stock_ship[n]->setPosition(75,0)->setSize(75, 30);
        weapons_stock_cargo[n] = new GuiLabel(weapons_layout[n],"","0/20",20);
        weapons_stock_cargo[n]->setPosition(150,0)->setSize(75, 30);

        weapons_stock_p1[n] = new GuiButton(weapons_layout[n],"","+ 1", [this, n]() {
            if (my_spaceship && selected_ship)
            {
                if (my_spaceship->getSystemEffectiveness(SYS_Docks) <= 0)
                    return;

                if (my_spaceship->weapon_storage[n] <= 0)
                {
                    my_spaceship->addToShipLog("Transfert de missile impossible. Aucun stock dans la station",colorConfig.log_generic,"docks");
                    return;
                }

                if (selected_ship->getWeaponStorageMax(EMissileWeapons(n)) == selected_ship->getWeaponStorage(EMissileWeapons(n)))
                {
                    my_spaceship->addToShipLog("Transfert de missile impossible. Stock maximum dans le drone",colorConfig.log_generic,"docks");
                    return;
                }

                my_spaceship->addToShipLog("Transfert de 1 " + getMissileWeaponName(EMissileWeapons(n)) + " Vers le drone",colorConfig.log_generic,"docks");

                my_spaceship->weapon_storage[n] -= 1;
                selected_ship->setWeaponStorage(EMissileWeapons(n), selected_ship->getWeaponStorage(EMissileWeapons(n)) + 1);
            }
        });
        weapons_stock_p1[n]->setSize(75, 40);

        weapons_stock_m1[n] = new GuiButton(weapons_layout[n],"","- 1", [this,n]() {
            if (my_spaceship)
            {
                if (my_spaceship->getSystemEffectiveness(SYS_Docks) <= 0)
                    return;

                if (selected_ship->getWeaponStorage(EMissileWeapons(n)) <= 0)
                {
                    my_spaceship->addToShipLog("Transfert de missile impossible. Aucun stock dans le drone",colorConfig.log_generic,"docks");
                    return;
                }

                if (my_spaceship->weapon_storage[n] == my_spaceship->weapon_storage_max[n])
                {
                    my_spaceship->addToShipLog("Transfert de missile impossible. Stock maximum dans la station",colorConfig.log_generic,"docks");
                    return;
                }

                my_spaceship->addToShipLog("Transfert de 1 " + getMissileWeaponName(EMissileWeapons(n)) + " vers la station",colorConfig.log_generic,"docks");

                my_spaceship->weapon_storage[n] += 1;
                selected_ship->setWeaponStorage(EMissileWeapons(n), selected_ship->getWeaponStorage(EMissileWeapons(n)) - 1);
            }
        });
        weapons_stock_m1[n]->setSize(75, 40);

        (new GuiPowerDamageIndicator(weapons_stock_p1[n], "DOCKS_DPI", SYS_Docks, ACenter, my_spaceship))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
        (new GuiPowerDamageIndicator(weapons_stock_m1[n], "DOCKS_DPI", SYS_Docks, ACenter, my_spaceship))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }

    model = new GuiRotatingModelView(topPanel, "MODEL_VIEW", nullptr);
    model->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setMargins(0, -100, 0, 0);

    (new GuiCustomShipFunctions(this, dockMaster, "CUSTOM_FUNCTIONS", my_spaceship))->setPosition(20, 550, ATopLeft)->setSize(360, GuiElement::GuiSizeMax);

    selectDock(0);
    model->moveToBack();
    background_crosses->moveToBack();

    new ShipsLog(this,"docks");
}

void DockMasterScreen::selectDock(int index_dock)
{
    this->index_dock = index_dock;
    docks->setSelectionIndex(index_dock);
    layout_move->setVisible(true);
}

void DockMasterScreen::selectBay(int index_bay)
{
    this->index_bay = index_bay;
    bays->setSelectionIndex(index_bay);
    layout_move->setVisible(true);

//    action_energy->setVisible(bays->getSelectionValue() == "energy");
//    action_weapons->setVisible(bays->getSelectionValue() == "Missile");
}

void DockMasterScreen::onDraw(sf::RenderTarget &window)
{
    GuiOverlay::onDraw(window);
    for (int n = 0; n < 10; n++)
        docks->setEntryName(n, "Vide");
    if (my_spaceship)
    {
        int n = 0;
        PVector<SpaceShip> list_ships;
        PVector<SpaceObject> list_objs = my_spaceship->getObjectsInRange(5000);
        foreach(SpaceObject, obj, list_objs)
        {
            P<SpaceShip> ship = obj;
            if (!ship)
                continue;
            if (ship == my_spaceship)
                continue;
            if (ship->docking_state != DS_Docked)
                continue;
            if (n > 10)
                return;

            // OK
            list_ships.push_back(ship);
            docks->setEntryName(n,ship->callsign);
            n +=1;
        }

        if (index_dock < list_ships.size())
            selected_ship = list_ships[index_dock];

        mainPanel->setVisible(false);
        foreach(SpaceShip,obj,list_ships){
            P<SpaceShip> ship = obj;
            if (ship->callsign == docks->getEntryName(index_dock)){
                displayShipDetails(ship);
                mainPanel->setVisible(true);
                return;
            }
        }

        // On r�cup�re l'info sur le dock s�lectionn�
//        P<SpaceShip> ship = list_ships[index];

//        if (ship)
//        {
//            action_move_selector->setSelectionIndex(action_move_selector->indexByValue(string(dockData.move_target_index)));
//            displayDroneDetails(dockData);
//            cancel_move_button->setVisible(false);
//            overlay->setVisible(false);
//            mainPanel->setVisible(true);
//        }else{
//            model->setModel(nullptr);
//            overlay->setVisible(true);
//            overlay_label->setText("Vide");
//            distance_bar->setVisible(false);
//            cancel_move_button->setVisible(false);
//            mainPanel->setVisible(false);
//        }
    }
    if (my_spaceship && selected_ship)
    {
        for(int n = 0; n < MW_Count; n++)
        {
            weapons_stock_cargo[n]->setText(string(selected_ship->getWeaponStorage(EMissileWeapons(n))) + " / " + string(selected_ship->getWeaponStorageMax(EMissileWeapons(n))));
            weapons_stock_ship[n]->setText(string(my_spaceship->getWeaponStorage(EMissileWeapons(n))) + " / " + string(my_spaceship->getWeaponStorageMax(EMissileWeapons(n))));
            weapons_stock_m1[n]->setEnable(selected_ship->getWeaponStorageMax(EMissileWeapons(n)) > 0 && my_spaceship->getWeaponStorageMax(EMissileWeapons(n)) > 0);
            weapons_stock_p1[n]->setEnable(selected_ship->getWeaponStorageMax(EMissileWeapons(n)) > 0 && my_spaceship->getWeaponStorageMax(EMissileWeapons(n)) > 0);
        }
    }
}

void DockMasterScreen::displayShipDetails(P<SpaceShip> ship)
{
    P<ShipTemplate> st = ship->ship_template;
    model->setModel(st->model_data);
    if (ship->id_dock == "")
        droneTitle->setText("Aucune baie d'ammarage");
    else
        droneTitle->setText("Baie d'ammarage " + ship->id_dock );
}
