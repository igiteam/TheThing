/* ================================================================
   File: console.c additions
   ================================================================ */

void RegisterCreatureLegCommands(void)
{
    // Creature legs system
    RegisterCommand("creature_legs", GL_Toggle, "Toggle creature-specific legs");
    RegisterCommand("creature_debug", GL_CreatureDebug, "Show creature legs debug info");
    RegisterCommand("test_creature", GL_TestCreatureSwitch, "Cycle test creatures");
    
    // Individual creature toggles
    RegisterCommand("marine_legs", Cmd_MarineLegs, "Toggle marine legs style");
    RegisterCommand("alien_legs", Cmd_AlienLegs, "Toggle alien legs style");
    RegisterCommand("predator_legs", Cmd_PredatorLegs, "Toggle predator legs style");
    
    // Creature-specific features
    RegisterCommand("alien_wallcrawl", Cmd_AlienWallcrawl, "Toggle alien wall-crawling");
    RegisterCommand("predator_cloak", Cmd_PredatorCloak, "Toggle predator cloak on legs");
}

void Cmd_MarineLegs(void)
{
    // Force marine leg style
    AvP.PlayerType = I_Marine;
    GL_DestroyCreatureLegs();
    GL_InitCreatureLegs();
    textprint("Switched to Marine legs\n");
}

void Cmd_AlienLegs(void)
{
    // Force alien leg style
    AvP.PlayerType = I_Alien;
    GL_DestroyCreatureLegs();
    GL_InitCreatureLegs();
    textprint("Switched to Alien legs (6 legs, wall-crawling)\n");
}

void Cmd_PredatorLegs(void)
{
    // Force predator leg style
    AvP.PlayerType = I_Predator;
    GL_DestroyCreatureLegs();
    GL_InitCreatureLegs();
    textprint("Switched to Predator legs (powerful stride, cloaking)\n");
}