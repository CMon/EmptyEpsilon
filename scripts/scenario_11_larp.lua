-- Name: Larp Test
-- Description: Empty scenario, no enemies, no friendlies. Just to show larp edition of Empty Epsilon
-- Type: Basic

function init()
    
    player = PlayerSpaceship():setFaction("Human Navy"):setTemplate("Battle docker")
    
    ------------------
    -- In progress ---
    ------------------
    
    ----
    -- weapons tube and beams specific by station
    ----
    
    -- the Battle docker have 3 beams and 5 tubes weapons. Example here : Beams are used only in station 1, the 2 first tubes are used only in station 2, the last 3 tubes are used only in station 3
    -- If there is noting in options.ini, weapons stations have all beams and tubes. You need to change options.ini to allow this feature.
    player:setBeamWeaponStation(0,1):setBeamWeaponStation(1,1):setBeamWeaponStation(2,1)
    player:setWeaponTubeStation(0,2):setWeaponTubeStation(1,2)
    player:setWeaponTubeStation(2,3):setWeaponTubeStation(3,3):setWeaponTubeStation(4,3)
    
    ----
    -- enable waypoints by route
    ----
    
    -- Example of waypoints for many routes
    player:commandAddWaypoint(20000,20000,1):commandAddWaypoint(-20000,-20000,2)
    
    ----
    -- dynamic texture files and texture color by player
    ----
    
    -- Change rgba color of the texture, example here for a dynamic background
    color_r = 1
    color_g = 1
    color_b = 1
    color_a = 1
    player:setTextureColor(color_r, color_g, color_b, color_a)
    
    -- Set the cubebox background (here not necessary, use the default string)
    player:setTexture("StarsFront", "StarsBack", "StarsLeft", "StarsRight", "StarsTop", "StarsBottom")

    ----------------------------------
    -- ### EE LARP release 1.0-beta --
    ----------------------------------
    
    ----
    -- GM access allowed from clients
    ----
    
    -- GM is available from client, with a password
    -- From now, only work with numbers
    setGMControlCode("123456")
    
    ----
    -- Advanced sector system
    ----
    player:setFarRangeRadarRange(100000000)
    addLocalName("0/0","Alpha.sec")
    addLocalName("0/0 1.1","Beta.sec")
    addLocalName("0/0 1.1 C2","Zeta.sec")

    ----
    -- Drones, fighters and docks system
    ----
    
    -- Example of adding a drone
    player:addDrone("L3 Mouse")
    
end

function update(delta)
    --No victory condition
    
    -- dynamic texture files and texture color by player
    color_r = color_r + random(-10, 10) / 100
    color_g = color_g + random(-10, 10) / 100
    color_b = color_b + random(-10, 10) / 100
    color_a = color_a + random(-10, 10) / 100
    player:setTextureColor(color_r, color_g, color_b, color_a)
end