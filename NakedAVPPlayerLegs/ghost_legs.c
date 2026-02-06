/* ================================================================
   File: ghost_legs.c
   JKDF2-style first-person legs for AvP Classic
   ================================================================ */

#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "player.h"
#include "gamedef.h"
#include "dynblock.h"

/* ================================================================
   Ghost Legs State Structure
   ================================================================ */
typedef struct GHOST_LEGS_SYSTEM
{
    DISPLAYBLOCK* pLegsDisplay;      // Ghost legs display block
    STRATEGYBLOCK* pLegsStrategy;    // Ghost legs strategy block
    int bActive;                     // Is system active?
    int bInitialized;                // Is system initialized?
    VECTORCH vOffset;                // Position offset from player
    int iTorsoJointIndex;            // Torso joint to hide (-1 if not found)
    int iUpdateCounter;              // Update counter for pulse system
    int iLastPlayerHealth;           // Last known player health
} GHOST_LEGS_SYSTEM;

static GHOST_LEGS_SYSTEM GhostLegs;
static int g_iFirstPersonLegsEnabled = 1; // CVar equivalent

/* ================================================================
   Initialize Ghost Legs System
   ================================================================ */
void GL_InitSystem(void)
{
    memset(&GhostLegs, 0, sizeof(GHOST_LEGS_SYSTEM));
    
    // Set default offset (slightly below camera for first-person)
    GhostLegs.vOffset = SetVector(0, -30 * ONE_FIXED, -60 * ONE_FIXED);
    
    GhostLegs.iTorsoJointIndex = -1;
    GhostLegs.iLastPlayerHealth = -1;
    
    // Initialize on next pulse
    GhostLegs.bInitialized = 1;
    
    #ifdef _DEBUG
    textprint("Ghost Legs System Initialized\n");
    #endif
}

/* ================================================================
   JKDF2-style Pulse System
   Similar to COG script's pulse message
   ================================================================ */
void GL_PulseSystem(void)
{
    static int iLastPulseTime = 0;
    int iCurrentTime = GetGameTime();
    
    // Pulse every 0.01 seconds (like JKDF2)
    if(iCurrentTime - iLastPulseTime < (ONE_FIXED / 100))
        return;
    
    iLastPulseTime = iCurrentTime;
    
    // Main update logic
    GL_Update();
}

/* ================================================================
   Main Update Function (JKDF2's "pulse" equivalent)
   ================================================================ */
void GL_Update(void)
{
    // Check if system should be active
    if(!g_iFirstPersonLegsEnabled || !Player || !Player->ObStrategyBlock)
    {
        GL_DestroyLegs();
        return;
    }
    
    PLAYER_STATUS* pPlayerStatus = (PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
    
    // Check conditions (like JKDF2's COG script)
    // 1. Player must be alive
    // 2. Must be in first-person view
    // 3. Must not be in cutscene/etc
    
    int bShouldShowLegs = GL_ShouldShowLegs(pPlayerStatus);
    
    if(bShouldShowLegs && !GhostLegs.bActive)
    {
        // Create legs (like JKDF2's CreateThing/FireProjectile)
        GL_CreateLegs();
    }
    else if(!bShouldShowLegs && GhostLegs.bActive)
    {
        // Destroy legs (like JKDF2's DestroyThing)
        GL_DestroyLegs();
    }
    
    // Update legs if active
    if(GhostLegs.bActive)
    {
        GL_UpdateLegsPosition();
        GL_UpdateLegsAnimation();
    }
}

/* ================================================================
   Should Show Legs? (JKDF2 COG logic)
   ================================================================ */
int GL_ShouldShowLegs(PLAYER_STATUS* pPlayerStatus)
{
    // Check player alive (like JKDF2's GetThingHealth check)
    if(!pPlayerStatus || !pPlayerStatus->IsAlive)
        return 0;
    
    // Store health for change detection
    int iCurrentHealth = Player->ObStrategyBlock->SBDamageBlock.Health;
    if(GhostLegs.iLastPlayerHealth == -1)
        GhostLegs.iLastPlayerHealth = iCurrentHealth;
    
    // If player just died, destroy legs immediately
    if(iCurrentHealth <= 0 && GhostLegs.iLastPlayerHealth > 0)
    {
        GL_DestroyLegs();
    }
    
    GhostLegs.iLastPlayerHealth = iCurrentHealth;
    
    // Check if in first-person (AvP doesn't have GetCurrentCamera like JKDF2)
    // We'll assume first-person when not observing and camera is normal
    if(MultiplayerObservedPlayer)
        return 0;
    
    // Check if looking down enough to see legs
    if(HeadOrientation.EulerX < 3072) // ~270 degrees (looking forward)
        return 0;
    
    return 1;
}

/* ================================================================
   Create Legs Entity (JKDF2's FireProjectile equivalent)
   ================================================================ */
void GL_CreateLegs(void)
{
    if(!Player || !Player->ObStrategyBlock)
        return;
    
    // Get player model (like JKDF2's GetThingModel)
    int iPlayerModel = Player->ObShape;
    
    // Create a new display block for legs
    GhostLegs.pLegsDisplay = CreateDisplayBlock();
    if(!GhostLegs.pLegsDisplay)
        return;
    
    // Copy player model/shape
    GhostLegs.pLegsDisplay->ObShape = iPlayerModel;
    GhostLegs.pLegsDisplay->ObFlags &= ~ObFlag_NotVis; // Make visible
    
    // Create strategy block for legs
    GhostLegs.pLegsStrategy = CreateStrategyBlock();
    if(!GhostLegs.pLegsStrategy)
    {
        FreeDisplayBlock(GhostLegs.pLegsDisplay);
        GhostLegs.pLegsDisplay = NULL;
        return;
    }
    
    // Link display and strategy blocks
    GhostLegs.pLegsDisplay->ObStrategyBlock = GhostLegs.pLegsStrategy;
    GhostLegs.pLegsStrategy->SBdptr = GhostLegs.pLegsDisplay;
    
    // Set as ghost type (no collision, like JKDF2's ghost template)
    GhostLegs.pLegsStrategy->I_SBtype = I_BehaviourGhost;
    
    // Copy player's HModel if available
    if(Player->HModelControlBlock)
    {
        GhostLegs.pLegsDisplay->HModelControlBlock = CloneHModel(Player->HModelControlBlock);
        
        // Find torso joint to hide (JKDF2's AmputateJoint(2))
        GL_FindAndHideTorso();
    }
    
    // Set physics flags (like JKDF2's SetPhysicsFlags)
    GhostLegs.pLegsDisplay->ObFlags |= ObFlag_NoCollision;
    GhostLegs.pLegsDisplay->ObFlags |= ObFlag_Ghost;
    
    // Attach to player (like JKDF2's AttachThingToThingEx)
    GhostLegs.pLegsDisplay->ObParent = Player;
    
    // Add to active block list for rendering
    AddToActiveBlockList(GhostLegs.pLegsDisplay);
    
    GhostLegs.bActive = 1;
    
    #ifdef _DEBUG
    textprint("First-person legs created\n");
    #endif
}

/* ================================================================
   Find and Hide Torso (JKDF2's AmputateJoint(2) equivalent)
   ================================================================ */
void GL_FindAndHideTorso(void)
{
    if(!GhostLegs.pLegsDisplay || !GhostLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    HMODELCONTROLLER* pHModel = GhostLegs.pLegsDisplay->HModelControlBlock;
    SECTION_DATA* pSectionData = pHModel->section_data;
    
    // Look for torso section (equivalent to joint 2 in JKDF2)
    while(pSectionData)
    {
        // Check section name for torso
        if(pSectionData->name && 
           (strstr(pSectionData->name, "torso") || 
            strstr(pSectionData->name, "chest") ||
            strstr(pSectionData->name, "spine")))
        {
            GhostLegs.iTorsoJointIndex = pSectionData->idx;
            
            // Hide it (AvP equivalent of AmputateJoint)
            pSectionData->flags |= SECTION_HIDDEN;
            
            #ifdef _DEBUG
            textprint("Hidden torso joint: %s (index %d)\n", 
                     pSectionData->name, pSectionData->idx);
            #endif
            break;
        }
        
        pSectionData = pSectionData->next;
    }
    
    // Also hide arms and head
    GL_HideUpperBodyParts();
}

/* ================================================================
   Hide Upper Body Parts
   ================================================================ */
void GL_HideUpperBodyParts(void)
{
    if(!GhostLegs.pLegsDisplay || !GhostLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    HMODELCONTROLLER* pHModel = GhostLegs.pLegsDisplay->HModelControlBlock;
    
    // Parts to hide (only leave legs visible)
    const char* partsToHide[] = {
        "head", "neck", "helmet",
        "arm_left", "arm_right", "shoulder_left", "shoulder_right",
        "hand_left", "hand_right",
        "weapon", "item",
        NULL
    };
    
    for(int i = 0; partsToHide[i]; i++)
    {
        SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, partsToHide[i]);
        if(pSection)
        {
            pSection->flags |= SECTION_HIDDEN;
        }
    }
}

/* ================================================================
   Update Legs Position (JKDF2's SetThingVel/Look equivalent)
   ================================================================ */
void GL_UpdateLegsPosition(void)
{
    if(!GhostLegs.bActive || !GhostLegs.pLegsDisplay || !Player)
        return;
    
    // Get player position and orientation
    VECTORCH vPlayerPos = Player->ObWorld;
    MATRIXCH mPlayerOrientation = Player->ObMat;
    
    // Calculate legs position (offset from player)
    VECTORCH vLegsPos = GhostLegs.vOffset;
    RotateVectorByMatrix(&vLegsPos, &mPlayerOrientation, &vLegsPos);
    vLegsPos = AddVectors(&vPlayerPos, &vLegsPos);
    
    // Set legs position
    GhostLegs.pLegsDisplay->ObWorld = vLegsPos;
    
    // Copy player orientation
    GhostLegs.pLegsDisplay->ObMat = mPlayerOrientation;
    
    // Copy player look vector (like JKDF2's GetThingLVec)
    GhostLegs.pLegsDisplay->ObEuler = Player->ObEuler;
    
    // Update velocity (for animation purposes)
    if(Player->ObStrategyBlock && Player->ObStrategyBlock->DynPtr)
    {
        DYNAMICSBLOCK* pDyn = Player->ObStrategyBlock->DynPtr;
        GhostLegs.pLegsDisplay->ObVel = pDyn->LinVelocity;
    }
    
    // Handle crouching (like JKDF2's IsThingCrouching check)
    PLAYER_STATUS* pPlayerStatus = (PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
    if(pPlayerStatus && pPlayerStatus->ShapeState == PMph_Crouching)
    {
        // Adjust legs for crouching
        GhostLegs.pLegsDisplay->ObWorld.vz -= 40 * ONE_FIXED;
    }
}

/* ================================================================
   Update Legs Animation
   ================================================================ */
void GL_UpdateLegsAnimation(void)
{
    if(!GhostLegs.bActive || !GhostLegs.pLegsDisplay || !Player)
        return;
    
    // Sync animation state with player
    GhostLegs.pLegsDisplay->ObAnimState = Player->ObAnimState;
    
    // Get player movement for leg swing
    if(Player->ObStrategyBlock && Player->ObStrategyBlock->DynPtr)
    {
        DYNAMICSBLOCK* pDyn = Player->ObStrategyBlock->DynPtr;
        int iSpeed = Approximate3dMagnitude(&pDyn->LinVelocity);
        
        // Simple leg swing based on speed (basic procedural animation)
        if(iSpeed > 1000)
        {
            GL_AnimateLegSwing(iSpeed);
        }
    }
}

/* ================================================================
   Simple Leg Swing Animation
   ================================================================ */
void GL_AnimateLegSwing(int iSpeed)
{
    if(!GhostLegs.pLegsDisplay || !GhostLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    static int iSwingPhase = 0;
    iSwingPhase += iSpeed / 1000;
    
    // Apply swing to leg joints
    HMODELCONTROLLER* pHModel = GhostLegs.pLegsDisplay->HModelControlBlock;
    
    // Find leg joints
    const char* legJoints[] = {"leg_left", "leg_right", NULL};
    
    for(int i = 0; legJoints[i]; i++)
    {
        SECTION_DATA* pLeg = GetThisSectionData(pHModel->section_data, legJoints[i]);
        if(pLeg)
        {
            // Calculate swing amount
            int iSwing = GetSin((iSwingPhase + (i * 32768)) & 65535);
            iSwing = MUL_FIXED(iSwing, iSpeed / 5000);
            
            // Apply rotation to leg
            // Note: This is simplified - real implementation would modify joint matrices
            pLeg->World_Offset.vy += iSwing;
        }
    }
}

/* ================================================================
   Destroy Legs (JKDF2's DestroyThing equivalent)
   ================================================================ */
void GL_DestroyLegs(void)
{
    if(!GhostLegs.bActive)
        return;
    
    // Remove from active block list
    if(GhostLegs.pLegsDisplay)
    {
        RemoveFromActiveBlockList(GhostLegs.pLegsDisplay);
        
        // Free HModel
        if(GhostLegs.pLegsDisplay->HModelControlBlock)
        {
            FreeHModel(GhostLegs.pLegsDisplay->HModelControlBlock);
        }
        
        FreeDisplayBlock(GhostLegs.pLegsDisplay);
    }
    
    // Free strategy block
    if(GhostLegs.pLegsStrategy)
    {
        FreeStrategyBlock(GhostLegs.pLegsStrategy);
    }
    
    // Reset state
    GhostLegs.pLegsDisplay = NULL;
    GhostLegs.pLegsStrategy = NULL;
    GhostLegs.bActive = 0;
    GhostLegs.iTorsoJointIndex = -1;
    
    #ifdef _DEBUG
    textprint("First-person legs destroyed\n");
    #endif
}

/* ================================================================
   Console Commands (for testing)
   ================================================================ */
void GL_Toggle(void)
{
    g_iFirstPersonLegsEnabled = !g_iFirstPersonLegsEnabled;
    
    if(!g_iFirstPersonLegsEnabled)
    {
        GL_DestroyLegs();
    }
    
    textprint("First-person legs: %s\n", 
        g_iFirstPersonLegsEnabled ? "ON" : "OFF");
}

void GL_Debug(void)
{
    textprint("=== First-Person Legs Debug ===\n");
    textprint("Enabled: %d\n", g_iFirstPersonLegsEnabled);
    textprint("Active: %d\n", GhostLegs.bActive);
    textprint("Initialized: %d\n", GhostLegs.bInitialized);
    textprint("Torso Joint Index: %d\n", GhostLegs.iTorsoJointIndex);
    
    if(GhostLegs.pLegsDisplay)
    {
        textprint("Legs Display: %p\n", GhostLegs.pLegsDisplay);
        textprint("Position: (%d, %d, %d)\n",
            GhostLegs.pLegsDisplay->ObWorld.vx,
            GhostLegs.pLegsDisplay->ObWorld.vy,
            GhostLegs.pLegsDisplay->ObWorld.vz);
    }
}