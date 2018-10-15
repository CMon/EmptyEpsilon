#include "main.h"
#include "autoConnectScreen.h"
#include "screens/mainScreen.h"
#include "preferenceManager.h"
#include "screenComponents/noiseOverlay.h"
#include "epsilonServer.h"
#include "gameGlobalInfo.h"
#include "playerInfo.h"

#include "gui/gui2_label.h"
#include "gui/gui2_overlay.h"

AutoConnectScreen::AutoConnectScreen(ECrewPosition crew_position, int auto_mainscreen, bool control_main_screen, string ship_filter)
: crew_position(crew_position), auto_mainscreen(auto_mainscreen), control_main_screen(control_main_screen)
{
    if (!game_client)
    {
        scanner = new ServerScanner(VERSION_NUMBER);
        scanner->scanLocalNetwork();
    }

    new GuiNoiseOverlay(this);

    status_label = new GuiLabel(this, "STATUS", "Coupure de courant", 50);
    status_label->setPosition(0, 300, ATopCenter)->setSize(0, 50);

    string position_name = "Ecran principal";
    if (crew_position < max_crew_positions)
        position_name = getCrewPositionName(crew_position);
    if (auto_mainscreen == 1)
        position_name = "Ecran principal";

    (new GuiLabel(this, "POSITION", position_name, 20))->setPosition(0, 10, ATopCenter)->setSize(0, 10);

    for(string filter : ship_filter.split(";"))
    {
        std::vector<string> key_value = filter.split("=", 1);
        string key = key_value[0].strip().lower();
        if (key.length() < 1)
            continue;

        if (key_value.size() == 1)
            ship_filters[key] = "1";
        else if (key_value.size() == 2)
            ship_filters[key] = key_value[1].strip();
        LOG(INFO) << "Auto connect filter: " << key << " = " << ship_filters[key];
    }

    if (PreferencesManager::get("instance_name") != "")
    {
        (new GuiLabel(this, "", PreferencesManager::get("instance_name"), 25))->setAlignment(ACenterLeft)->setPosition(20, 20, ATopLeft)->setSize(0, 18);
    }
}

AutoConnectScreen::~AutoConnectScreen()
{
    if (scanner)
        scanner->destroy();
}

void AutoConnectScreen::update(float delta)
{
    if (scanner)
    {
        std::vector<ServerScanner::ServerInfo> serverList = scanner->getServerList();

        if (serverList.size() > 0)
        {
            status_label->setText("Found server " + serverList[0].name);
            connect_to_address = serverList[0].address;
            new GameClient(VERSION_NUMBER, serverList[0].address);
            scanner->destroy();
        }else{
            status_label->setText("Coupure de courant");
        }
    }else{
        switch(game_client->getStatus())
        {
        case GameClient::ReadyToConnect:
        case GameClient::Connecting:
        case GameClient::Authenticating:
            status_label->setText("Connexion: " + connect_to_address.toString());
            break;
        case GameClient::WaitingForPassword: //For now, just disconnect when we found a password protected server.
        case GameClient::Disconnected:
            disconnectFromServer();
            scanner = new ServerScanner(VERSION_NUMBER);
            scanner->scanLocalNetwork();
            break;
        case GameClient::Connected:
            if (game_client->getClientId() > 0)
            {
                foreach(PlayerInfo, i, player_info_list)
                    if (i->client_id == game_client->getClientId())
                        my_player_info = i;
                if (my_player_info && gameGlobalInfo)
                {
                    status_label->setText("Retablissement du systeme...");
                    if (!my_spaceship)
                    {
                        for(int n=0; n<GameGlobalInfo::max_player_ships; n++)
                        {
                            if (isValidShip(n))
                            {
                                connectToShip(n);
                                break;
                            }
                        }
                    }else{
                        if (my_spaceship->getMultiplayerId() == my_player_info->ship_id && (auto_mainscreen == 1 || crew_position == max_crew_positions || my_player_info->crew_position[crew_position]))
                        {
                            if (auto_mainscreen == 1)
                            {
                                for(int n=0; n<max_crew_positions; n++)
                                    my_player_info->commandSetCrewPosition(crew_position, false);
                            }
                            destroy();
                            my_player_info->spawnUI();
                        }
                    }
                }else{
                    status_label->setText("Connexion en cours...");
                }
            }
            break;
        }
    }
}

bool AutoConnectScreen::isValidShip(int index)
{
    P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index);

    if (!ship || !ship->ship_template)
        return false;

    for(auto it : ship_filters)
    {
        if (it.first == "solo")
        {
            int crew_at_position = 0;
            foreach(PlayerInfo, i, player_info_list)
            {
                if (i->ship_id == ship->getMultiplayerId())
                {
                    if (crew_position != max_crew_positions && i->crew_position[crew_position])
                        crew_at_position++;
                }
            }
            if (crew_at_position > 0)
                return false;
        }
        else if (it.first == "faction")
        {
            if (ship->getFactionId() != FactionInfo::findFactionId(it.second))
                return false;
        }
        else if (it.first == "callsign")
        {
            if (ship->getCallSign().lower() != it.second.lower())
                return false;
        }
        else if (it.first == "type")
        {
            if (ship->getTypeName().lower() != it.second.lower())
                return false;
        }
        else
        {
            LOG(WARNING) << "Unknown ship filter: " << it.first << " = " << it.second;
        }
    }
    return true;
}

void AutoConnectScreen::connectToShip(int index)
{
    P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index);

    if (auto_mainscreen != 1 && crew_position != max_crew_positions)    //If we are not the main screen, setup the right crew position.
    {
        my_player_info->commandSetCrewPosition(crew_position, true);
        my_player_info->commandSetMainScreenControl(control_main_screen);
    }
    my_player_info->commandSetShipId(ship->getMultiplayerId());
}
