-- File: Scripts/Game/GameRules.lua modifications

function GameRules:OnClientConnect(channelId, reset)
    -- ... existing code ...
    
    -- Initialize first-person legs for the player
    FirstPersonLegs:Startup()
    
    -- ... rest of existing code ...
end

function GameRules:OnClientDisconnect(channelId)
    -- ... existing code ...
    
    -- Clean up first-person legs
    FirstPersonLegs:DestroyLegs()
    
    -- ... rest of existing code ...
end