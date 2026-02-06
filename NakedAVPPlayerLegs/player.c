/* ================================================================
   File: player.c modifications
   ================================================================ */

// In MaintainPlayer() function:
void MaintainPlayer(void)
{
    // ... existing code ...
    
    // Initialize ghost legs system on first run
    static int bLegsInitialized = 0;
    if(!bLegsInitialized && playerStatusPtr->IsAlive)
    {
        GL_InitSystem();
        bLegsInitialized = 1;
    }
    
    // Update ghost legs system (pulse-based like JKDF2)
    if(bLegsInitialized && g_iFirstPersonLegsEnabled)
    {
        GL_PulseSystem();
    }
    
    // ... rest of existing code ...
}

// Also need to handle player death/respawn:
void PlayerIsDead(DAMAGE_PROFILE* damage,int multiplier,VECTORCH* incoming)
{
    // ... existing death code ...
    
    // Destroy legs when player dies
    GL_DestroyLegs();
    
    // ... rest of existing code ...
}