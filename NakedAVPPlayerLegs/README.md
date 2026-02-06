🚀 Quick Start Instructions
For AvP Classic:
Add ghost_legs.c to your project
Add #include "ghost_legs.h" to player.c
Call GL_InitSystem() in InitPlayer()
Call GL_PulseSystem() in MaintainPlayer()
Add console commands
Compile and test

For Far Cry 1:
Create Scripts/Game/FirstPersonLegs.lua
Create Scripts/Entities/FirstPersonLegs.xml
Add to GameRules.lua initialization
Start game and use fp_legs console command
Tweak offset/visibility in Lua (no recompile needed!)

💡 Key Advantages of This Approach
No engine modifications (for Far Cry)
Uses existing systems (ghost entities, attachments)
Easy to disable if performance issues
Player customization possible (different leg styles)
Multiplayer compatible (client-side only)

⚠️ Potential Issues & Solutions
Issue AvP Solution Far Cry Solution
Z-fighting Adjust offset Use depth bias in material
Animation sync Copy anim state Use animation events
Performance Limit updates Use LOD, distance culling
Multiplayer Client-side only Network sync optional

The JKDF2 COG script approach is perfect for both games - it's a simple, elegant solution that works within existing game systems without requiring deep engine modifications.
