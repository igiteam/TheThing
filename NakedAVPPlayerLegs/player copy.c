/* ================================================================
   File: player.c modifications for creature legs
   ================================================================ */

void MaintainPlayer(void)
{
    // ... existing code ...
    
    // Initialize creature legs on first run
    static int bCreatureLegsInitialized = 0;
    if(!bCreatureLegsInitialized && playerStatusPtr->IsAlive)
    {
        GL_InitCreatureLegs();
        bCreatureLegsInitialized = 1;
    }
    
    // Check if creature changed (e.g., through cheat or multiplayer)
    static int lastCreatureType = -1;
    if(lastCreatureType != AvP.PlayerType)
    {
        // Creature changed, reinitialize legs
        GL_DestroyCreatureLegs();
        GL_InitCreatureLegs();
        lastCreatureType = AvP.PlayerType;
    }
    
    // Update creature legs
    if(bCreatureLegsInitialized && g_iCreatureLegsEnabled)
    {
        GL_PulseSystem(); // Same pulse system from earlier
        
        // Creature-specific updates
        if(EnhancedLegs.bActive)
        {
            GL_AnimateCreatureLegs();
            
            // Special creature effects
            switch(AvP.PlayerType)
            {
                case I_Alien:
                    // Alien wall-crawling effects
                    if(playerStatusPtr->ShapeState == PMph_Crouching)
                    {
                        UpdateAlienWallEffects();
                    }
                    break;
                    
                case I_Predator:
                    // Predator cloak effects
                    if(playerStatusPtr->ProcWalk.cloakOn)
                    {
                        UpdatePredatorCloakEffects();
                    }
                    break;
            }
        }
    }
    
    // ... rest of existing code ...
}

/* ================================================================
   Creature-Specific Sound Effects
   ================================================================ */
void PlayCreatureFootstepSounds(void)
{
    if(!EnhancedLegs.bActive)
        return;
    
    // Check for foot plants (when leg phase resets)
    static int lastLegPhases[6] = {0};
    
    for(int i = 0; i < EnhancedLegs.pCurrentConfig->iLegCount; i++)
    {
        // Check if leg just planted (phase wrapped around)
        if(EnhancedLegs.iLegPhases[i] < lastLegPhases[i])
        {
            // Foot planted - play sound
            PlayCreatureFootstep(i);
        }
        
        lastLegPhases[i] = EnhancedLegs.iLegPhases[i];
    }
}

void PlayCreatureFootstep(int legIndex)
{
    VECTORCH soundPos = Player->ObWorld;
    
    // Adjust position based on which leg
    switch(EnhancedLegs.iCreatureType)
    {
        case I_Marine:
            Sound_Play(SID_MARINE_FOOTSTEP, "d", &soundPos);
            break;
            
        case I_Alien:
            // Alien has different footstep sounds
            switch(legIndex % 3)
            {
                case 0: Sound_Play(SID_ALIEN_FOOTSTEP_FRONT, "d", &soundPos); break;
                case 1: Sound_Play(SID_ALIEN_FOOTSTEP_MID, "d", &soundPos); break;
                case 2: Sound_Play(SID_ALIEN_FOOTSTEP_BACK, "d", &soundPos); break;
            }
            break;
            
        case I_Predator:
            // Predator has heavy, metallic footsteps
            Sound_Play(SID_PREDATOR_FOOTSTEP, "d", &soundPos);
            break;
    }
}