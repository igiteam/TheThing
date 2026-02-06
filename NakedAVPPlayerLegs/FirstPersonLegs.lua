-- ================================================================
-- File: Scripts/Game/FirstPersonLegs.lua
-- JKDF2-style first-person legs for Far Cry 1
-- ================================================================

FirstPersonLegs = {
    -- State variables (like JKDF2 COG symbols)
    Player = nil,
    PlayerModel = nil,
    Legs = nil,
    LegsTemplate = "firstperson_legs",
    
    -- Configuration (like JKDF2 COG constants)
    Enabled = 1,
    PulseRate = 0.01,  -- 100Hz like JKDF2
    HealthCheckDelay = 0.25,
    
    -- Internal state
    bInitialized = false,
    bLegsCreated = false,
    LastHealth = 100,
    PulseTimer = 0
}

-- ================================================================
-- Startup Message (JKDF2's "startup" equivalent)
-- ================================================================
function FirstPersonLegs:Startup()
    if self.Enabled == 0 then
        return
    end
    
    -- Get local player (JKDF2's GetLocalPlayerThing)
    self.Player = System.GetLocalPlayer()
    
    if not self.Player then
        -- Sleep while level loads (JKDF2's Sleep(0.25))
        self:Schedule(function()
            System.Log("FirstPersonLegs: Waiting for player...")
            self:Startup()
        end, self.HealthCheckDelay)
        return
    end
    
    -- Get player model (JKDF2's GetThingModel)
    local playerEntity = self.Player:GetEntity()
    if playerEntity then
        local character = playerEntity:GetCharacter(0)
        if character then
            self.PlayerModel = character:GetFilePath()
        end
    end
    
    -- Set pulse rate (JKDF2's SetPulse(0.01))
    self.PulseTimer = 0
    self.bInitialized = true
    
    System.Log("FirstPersonLegs: System initialized")
end

-- ================================================================
-- Pulse Message (JKDF2's "pulse" equivalent)
-- ================================================================
function FirstPersonLegs:Pulse(deltaTime)
    if not self.bInitialized or self.Enabled == 0 then
        return
    end
    
    self.PulseTimer = self.PulseTimer + deltaTime
    if self.PulseTimer < self.PulseRate then
        return
    end
    self.PulseTimer = 0
    
    -- Main update logic (like JKDF2 COG pulse)
    self:Update()
end

-- ================================================================
-- Update Function (JKDF2 COG pulse logic)
-- ================================================================
function FirstPersonLegs:Update()
    -- Check camera mode (JKDF2's GetCurrentCamera() == 1)
    local bThirdPerson = (self.Player:GetViewMode() == 3)  -- 3 = third person
    
    -- Check health (JKDF2's GetThingHealth(Player) <= 0)
    local currentHealth = self.Player:GetHealth()
    
    if bThirdPerson or currentHealth <= 0 then
        if self.bLegsCreated then
            self:DestroyLegs()
        end
        return
    end
    
    -- Check if looking down to see legs
    local viewAngles = self.Player:GetViewAngles()
    local bLookingDown = viewAngles.x > 0.3  -- ~17 degrees
    
    if not bLookingDown then
        if self.bLegsCreated then
            self:DestroyLegs()
        end
        return
    end
    
    -- Create legs if needed
    if not self.bLegsCreated then
        self:CreateLegs()
    end
    
    -- Update legs if active
    if self.bLegsCreated and self.Legs then
        self:UpdateLegs()
    end
end

-- ================================================================
-- Create Legs (JKDF2's FireProjectile equivalent)
-- ================================================================
function FirstPersonLegs:CreateLegs()
    if not self.Player then
        return
    end
    
    local playerEntity = self.Player:GetEntity()
    if not playerEntity then
        return
    end
    
    -- Create ghost entity (JKDF2's FireProjectile)
    local spawnParams = {
        class = "GhostEntity",
        name = "FirstPersonLegs",
        position = playerEntity:GetWorldPos(),
        orientation = playerEntity:GetWorldRotation(),
        flags = bor(ENTITY_FLAG_CLIENT_ONLY, ENTITY_FLAG_NO_SAVE),
        properties = {
            bPickable = 0,
            bUsable = 0,
            bAutoGenAIHidePts = 0,
            bAutoGenAICoverPts = 0,
        }
    }
    
    -- Spawn the legs entity
    self.Legs = System.SpawnEntity(spawnParams)
    if not self.Legs then
        System.Log("FirstPersonLegs: Failed to spawn legs entity")
        return
    end
    
    -- Load player model (JKDF2's SetThingModel)
    if self.PlayerModel then
        self.Legs:LoadCharacter(0, self.PlayerModel)
    end
    
    -- Hide torso and upper body (JKDF2's AmputateJoint(2))
    self:HideUpperBody()
    
    -- Attach to player (JKDF2's AttachThingToThingEx with flag 0x8)
    self.Legs:SetParent(playerEntity.id)
    
    -- Set as ghost (no physics, no collision)
    local physParams = {
        type = PE_NONE,  -- No physics
    }
    self.Legs:Physicalize(physParams)
    
    -- Set render flags (only visible to local player)
    self.Legs:SetViewDistRatio(0)  -- 0 = only render for local player
    
    self.bLegsCreated = true
    System.Log("FirstPersonLegs: Legs created")
end

-- ================================================================
-- Hide Upper Body (JKDF2's AmputateJoint equivalent)
-- ================================================================
function FirstPersonLegs:HideUpperBody()
    if not self.Legs then
        return
    end
    
    local character = self.Legs:GetCharacter(0)
    if not character then
        return
    end
    
    local attachmentMgr = character:GetIAttachmentManager()
    if not attachmentMgr then
        return
    end
    
    -- Hide torso attachments (joint 2 in JKDF2)
    -- In Far Cry, we hide specific bone attachments
    local partsToHide = {
        "Bip01 Spine1",     -- Upper spine (torso)
        "Bip01 Spine2",     -- Chest
        "Bip01 Spine3",     -- Lower chest
        "Bip01 Neck",       -- Neck
        "Bip01 Head",       -- Head
        "Bip01 L Clavicle", -- Left shoulder
        "Bip01 R Clavicle", -- Right shoulder
        "Bip01 L UpperArm", -- Left upper arm
        "Bip01 R UpperArm", -- Right upper arm
        "Bip01 L Forearm",  -- Left forearm
        "Bip01 R Forearm",  -- Right forearm
        "Bip01 L Hand",     -- Left hand
        "Bip01 R Hand",     -- Right hand
        "weapon",           -- Weapon attachment
    }
    
    for _, partName in ipairs(partsToHide) do
        local attachment = attachmentMgr:GetInterfaceByName(partName)
        if attachment then
            attachment:HideAttachment(1)  -- Hide completely
        end
    end
    
    -- Also hide via material transparency for parts without attachments
    self:HideViaMaterial()
end

-- ================================================================
-- Hide via Material Transparency
-- ================================================================
function FirstPersonLegs:HideViaMaterial()
    if not self.Legs then
        return
    end
    
    -- Create a transparent material for hidden parts
    local transparentMat = Material.CreateMaterial({
        shader = "Illum",
        textures = {
            diffuse = "textures/common/transparent.dds",
        },
        flags = {
            nopreview = 1,
            notemplate = 1,
        },
        publicParams = {
            Alpha = 0.0,  -- Fully transparent
        }
    })
    
    -- Apply to upper body sub-materials
    local character = self.Legs:GetCharacter(0)
    if character then
        local skin = character:GetISkin()
        if skin then
            -- These material IDs might need adjustment per character
            local materialsToHide = {2, 3, 4, 5}  -- Torso, arms, head materials
            for _, matId in ipairs(materialsToHide) do
                skin:SetMaterial(matId, transparentMat)
            end
        end
    end
end

-- ================================================================
-- Update Legs (JKDF2's SetThingVel/SetThingLook equivalent)
-- ================================================================
function FirstPersonLegs:UpdateLegs()
    if not self.Legs or not self.Player then
        return
    end
    
    local playerEntity = self.Player:GetEntity()
    if not playerEntity then
        return
    end
    
    -- Update position (slightly below player for first-person)
    local playerPos = playerEntity:GetWorldPos()
    local playerRot = playerEntity:GetWorldRotation()
    
    -- Offset (like JKDF2's position adjustment)
    local offset = Vec3(0, -0.2, -0.5)  -- Behind and below camera
    
    -- Transform offset by player rotation
    offset = playerRot:TransformVector(offset)
    local legsPos = playerPos + offset
    
    self.Legs:SetWorldPos(legsPos)
    self.Legs:SetWorldRotation(playerRot)
    
    -- Copy velocity (JKDF2's SetThingVel)
    local velocity = playerEntity:GetVelocity()
    
    -- Handle crouching (JKDF2's IsThingCrouching check)
    if self.Player:IsCrouching() then
        -- Adjust for crouch
        legsPos.z = legsPos.z - 0.3
        self.Legs:SetWorldPos(legsPos)
    end
    
    -- Simple leg animation based on movement
    self:AnimateLegs(velocity)
end

-- ================================================================
-- Animate Legs (Basic procedural animation)
-- ================================================================
function FirstPersonLegs:AnimateLegs(velocity)
    if not self.Legs then
        return
    end
    
    local speed = velocity:len()
    local character = self.Legs:GetCharacter(0)
    if not character or speed < 0.1 then
        return
    end
    
    local skeletonPose = character:GetISkeletonPose()
    if not skeletonPose then
        return
    end
    
    -- Get current time for animation
    local time = System.GetCurrTime()
    
    -- Simple leg swing animation
    local swingAmount = math.sin(time * speed * 5) * 0.1
    
    -- Left leg
    local leftLegId = skeletonPose:GetJointIDByName("Bip01 L Thigh")
    if leftLegId >= 0 then
        local leftTransform = skeletonPose:GetAbsJointByID(leftLegId)
        leftTransform.q = Quat:CreateRotationX(swingAmount)
        skeletonPose:SetAbsJointByID(leftLegId, leftTransform)
    end
    
    -- Right leg (180 degrees out of phase)
    local rightLegId = skeletonPose:GetJointIDByName("Bip01 R Thigh")
    if rightLegId >= 0 then
        local rightTransform = skeletonPose:GetAbsJointByID(rightLegId)
        rightTransform.q = Quat:CreateRotationX(-swingAmount)
        skeletonPose:SetAbsJointByID(rightLegId, rightTransform)
    end
end

-- ================================================================
-- Destroy Legs (JKDF2's DestroyThing equivalent)
-- ================================================================
function FirstPersonLegs:DestroyLegs()
    if not self.bLegsCreated or not self.Legs then
        return
    end
    
    -- Remove entity
    System.RemoveEntity(self.Legs.id)
    self.Legs = nil
    self.bLegsCreated = false
    
    System.Log("FirstPersonLegs: Legs destroyed")
end

-- ================================================================
-- Console Commands (JKDF2-style testing)
-- ================================================================
function FirstPersonLegs.Toggle()
    if FirstPersonLegs.Enabled == 1 then
        FirstPersonLegs.Enabled = 0
        FirstPersonLegs:DestroyLegs()
        System.Log("FirstPersonLegs: OFF")
    else
        FirstPersonLegs.Enabled = 1
        FirstPersonLegs:Startup()
        System.Log("FirstPersonLegs: ON")
    end
end

function FirstPersonLegs.Debug()
    System.Log("=== FirstPersonLegs Debug ===")
    System.Log("Enabled: " .. tostring(FirstPersonLegs.Enabled))
    System.Log("Initialized: " .. tostring(FirstPersonLegs.bInitialized))
    System.Log("Legs Created: " .. tostring(FirstPersonLegs.bLegsCreated))
    
    if FirstPersonLegs.Player then
        local health = FirstPersonLegs.Player:GetHealth()
        local viewMode = FirstPersonLegs.Player:GetViewMode()
        System.Log("Player Health: " .. health)
        System.Log("View Mode: " .. viewMode .. " (1=FP, 3=TP)")
    end
end

-- ================================================================
-- System Registration
-- ================================================================

-- Register for updates (similar to JKDF2's pulse system)
Script.SetUpdateHandler(FirstPersonLegs)

-- Auto-start on game load
function OnGameStart()
    FirstPersonLegs:Startup()
end

-- Console commands
System.AddCCommand("fp_legs", FirstPersonLegs.Toggle, "Toggle first-person legs")
System.AddCCommand("fp_legs_debug", FirstPersonLegs.Debug, "Show first-person legs debug info")

-- Register game start callback
Script.RegisterCallback("OnGameStart", OnGameStart)