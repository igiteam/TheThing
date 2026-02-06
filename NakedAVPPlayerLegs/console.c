/* ================================================================
   File: console.c modifications
   ================================================================ */

void RegisterConsoleCommands(void)
{
    // ... existing commands ...
    
    // First-person legs commands
    RegisterCommand("fp_legs", GL_Toggle, "Toggle first-person legs");
    RegisterCommand("fp_legs_debug", GL_Debug, "Show first-person legs debug info");
    
    // ... rest of existing commands ...
}