/* ================================================================
   File: ghost_legs_creatures.c
   Creature-specific first-person legs for AvP Classic
   ================================================================ */

#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "player.h"
#include "gamedef.h"
#include "dynblock.h"
#include "weapons.h"

/* ================================================================
   Creature-Specific Leg Definitions
   ================================================================ */
typedef struct CREATURE_LEG_CONFIG
{
    // Basic settings
    VECTORCH vOffset;                // Position offset
    VECTORCH vLegOffsetLeft;         // Left leg position offset
    VECTORCH vLegOffsetRight;        // Right leg position offset
    int iLegCount;                   // Number of legs (2, 4, 6)
    
    // Joint names to hide (creature-specific)
    const char** pJointsToHide;
    int iNumJointsToHide;
    
    // Joint names to keep visible (legs)
    const char** pLegJoints;
    int iNumLegJoints;
    
    // Animation parameters
    int iSwingMultiplier;           // Leg swing intensity
    int iStepHeight;                // Max step climb
    int bCanCrawlWalls;             // Wall-crawling ability
} CREATURE_LEG_CONFIG;

/* ================================================================
   Marine Configuration (Human Biped)
   ================================================================ */
static const char* MarineJointsToHide[] = {
    "torso", "chest", "spine1", "spine2",
    "neck", "head", "helmet",
    "shoulder_left", "shoulder_right",
    "arm_left", "arm_right",
    "hand_left", "hand_right",
    "weapon_attach", "item_attach",
    NULL
};

static const char* MarineLegJoints[] = {
    "leg_left_upper", "leg_left_lower", "leg_left_foot",
    "leg_right_upper", "leg_right_lower", "leg_right_foot",
    "pelvis", "hip_left", "hip_right",
    NULL
};

static CREATURE_LEG_CONFIG MarineConfig = {
    .vOffset = {0, -30 * ONE_FIXED, -60 * ONE_FIXED}, // Behind and below
    .vLegOffsetLeft = {-20 * ONE_FIXED, 40 * ONE_FIXED, 0},
    .vLegOffsetRight = {20 * ONE_FIXED, 40 * ONE_FIXED, 0},
    .iLegCount = 2,
    .pJointsToHide = MarineJointsToHide,
    .iNumJointsToHide = 14,
    .pLegJoints = MarineLegJoints,
    .iNumLegJoints = 9,
    .iSwingMultiplier = 12000,       // Moderate leg swing
    .iStepHeight = 18 * ONE_FIXED,   // 18 unit step
    .bCanCrawlWalls = 0
};

/* ================================================================
   Alien Configuration (Xenomorph - 6 legs)
   ================================================================ */
static const char* AlienJointsToHide[] = {
    "torso", "chest", "spine", "spine1", "spine2", "spine3",
    "neck", "head", "jaw", "teeth",
    "arm_left", "arm_right", "claw_left", "claw_right",
    "tail_base", "tail_mid", "tail_tip",
    NULL
};

static const char* AlienLegJoints[] = {
    // 6 legs: front, middle, back pairs
    "leg_front_left_upper", "leg_front_left_lower", "leg_front_left_foot",
    "leg_front_right_upper", "leg_front_right_lower", "leg_front_right_foot",
    "leg_mid_left_upper", "leg_mid_left_lower", "leg_mid_left_foot",
    "leg_mid_right_upper", "leg_mid_right_lower", "leg_mid_right_foot",
    "leg_back_left_upper", "leg_back_left_lower", "leg_back_left_foot",
    "leg_back_right_upper", "leg_back_right_lower", "leg_back_right_foot",
    "pelvis", "abdomen",
    NULL
};

static CREATURE_LEG_CONFIG AlienConfig = {
    .vOffset = {0, -40 * ONE_FIXED, -80 * ONE_FIXED}, // Lower for crouched stance
    .vLegOffsetLeft = {-30 * ONE_FIXED, 60 * ONE_FIXED, -20 * ONE_FIXED},
    .vLegOffsetRight = {30 * ONE_FIXED, 60 * ONE_FIXED, -20 * ONE_FIXED},
    .iLegCount = 6,
    .pJointsToHide = AlienJointsToHide,
    .iNumJointsToHide = 17,
    .pLegJoints = AlienLegJoints,
    .iNumLegJoints = 20,
    .iSwingMultiplier = 8000,        // Subtle leg swing (crouched)
    .iStepHeight = 12 * ONE_FIXED,   // Lower step (crawls over)
    .bCanCrawlWalls = 1
};

/* ================================================================
   Predator Configuration (Tall Biped)
   ================================================================ */
static const char* PredatorJointsToHide[] = {
    "torso", "chest", "spine", "spine1", "spine2",
    "neck", "head", "mask", "dreads",
    "shoulder_left", "shoulder_right",
    "arm_left", "arm_right",
    "wrist_left", "wrist_right", "wristblade_left", "wristblade_right",
    "hand_left", "hand_right",
    "backpack", "plasmacaster", "shouldercannon",
    "belt", "trophy_rack",
    NULL
};

static const char* PredatorLegJoints[] = {
    "leg_left_upper", "leg_left_lower", "leg_left_foot",
    "leg_right_upper", "leg_right_lower", "leg_right_foot",
    "pelvis", "hip_left", "hip_right",
    "knee_left", "knee_right",
    "boot_left", "boot_right",
    NULL
};

static CREATURE_LEG_CONFIG PredatorConfig = {
    .vOffset = {0, -50 * ONE_FIXED, -100 * ONE_FIXED}, // Taller, further back
    .vLegOffsetLeft = {-25 * ONE_FIXED, 50 * ONE_FIXED, 10 * ONE_FIXED},
    .vLegOffsetRight = {25 * ONE_FIXED, 50 * ONE_FIXED, 10 * ONE_FIXED},
    .iLegCount = 2,
    .pJointsToHide = PredatorJointsToHide,
    .iNumJointsToHide = 21,
    .pLegJoints = PredatorLegJoints,
    .iNumLegJoints = 13,
    .iSwingMultiplier = 15000,       // Powerful leg swing
    .iStepHeight = 24 * ONE_FIXED,   // High step
    .bCanCrawlWalls = 0
};

/* ================================================================
   Enhanced Ghost Legs System with Creature Support
   ================================================================ */
typedef struct ENHANCED_GHOST_LEGS
{
    DISPLAYBLOCK* pLegsDisplay;
    STRATEGYBLOCK* pLegsStrategy;
    CREATURE_LEG_CONFIG* pCurrentConfig;
    
    // Creature-specific state
    int iCreatureType;              // I_Marine, I_Alien, I_Predator
    int bActive;
    int bInitialized;
    
    // Animation state
    int iLegPhases[6];              // Phase for each leg (0-65535)
    int iSwingIntensity[6];         // Swing amount for each leg
    VECTORCH vLegPositions[6];      // Current leg positions
    VECTORCH vTargetLegPositions[6]; // Target leg positions
    
    // Wall-crawling state (Alien only)
    int bOnWall;
    VECTORCH vWallNormal;
    int iWallSurfaceType;
} ENHANCED_GHOST_LEGS;

static ENHANCED_GHOST_LEGS EnhancedLegs;
static int g_iCreatureLegsEnabled = 1;

/* ================================================================
   Get Current Creature Configuration
   ================================================================ */
CREATURE_LEG_CONFIG* GL_GetCreatureConfig(void)
{
    if(!Player || !Player->ObStrategyBlock)
        return &MarineConfig; // Default
    
    PLAYER_STATUS* pStatus = (PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
    
    switch(AvP.PlayerType)
    {
        case I_Marine:
            EnhancedLegs.iCreatureType = I_Marine;
            return &MarineConfig;
            
        case I_Alien:
            EnhancedLegs.iCreatureType = I_Alien;
            return &AlienConfig;
            
        case I_Predator:
            EnhancedLegs.iCreatureType = I_Predator;
            return &PredatorConfig;
            
        default:
            return &MarineConfig;
    }
}

/* ================================================================
   Initialize Creature-Specific Legs
   ================================================================ */
void GL_InitCreatureLegs(void)
{
    memset(&EnhancedLegs, 0, sizeof(ENHANCED_GHOST_LEGS));
    
    // Get current creature config
    EnhancedLegs.pCurrentConfig = GL_GetCreatureConfig();
    
    // Initialize leg phases (staggered for natural gait)
    for(int i = 0; i < 6; i++)
    {
        EnhancedLegs.iLegPhases[i] = (i * 65536) / EnhancedLegs.pCurrentConfig->iLegCount;
        EnhancedLegs.iSwingIntensity[i] = 0;
        EnhancedLegs.vLegPositions[i] = SetVector(0, 0, 0);
        EnhancedLegs.vTargetLegPositions[i] = SetVector(0, 0, 0);
    }
    
    EnhancedLegs.bInitialized = 1;
    
    #ifdef _DEBUG
    textprint("Creature legs initialized: Type %d, %d legs\n",
        EnhancedLegs.iCreatureType, EnhancedLegs.pCurrentConfig->iLegCount);
    #endif
}

/* ================================================================
   Create Creature-Specific Legs
   ================================================================ */
void GL_CreateCreatureLegs(void)
{
    if(!Player || EnhancedLegs.bActive)
        return;
    
    // Get current configuration
    EnhancedLegs.pCurrentConfig = GL_GetCreatureConfig();
    
    // Create display block
    EnhancedLegs.pLegsDisplay = CreateDisplayBlock();
    if(!EnhancedLegs.pLegsDisplay)
        return;
    
    // Copy player model
    EnhancedLegs.pLegsDisplay->ObShape = Player->ObShape;
    EnhancedLegs.pLegsDisplay->ObFlags &= ~ObFlag_NotVis;
    
    // Create strategy block
    EnhancedLegs.pLegsStrategy = CreateStrategyBlock();
    if(!EnhancedLegs.pLegsStrategy)
    {
        FreeDisplayBlock(EnhancedLegs.pLegsDisplay);
        EnhancedLegs.pLegsDisplay = NULL;
        return;
    }
    
    // Link blocks
    EnhancedLegs.pLegsDisplay->ObStrategyBlock = EnhancedLegs.pLegsStrategy;
    EnhancedLegs.pLegsStrategy->SBdptr = EnhancedLegs.pLegsDisplay;
    EnhancedLegs.pLegsStrategy->I_SBtype = I_BehaviourGhost;
    
    // Copy HModel
    if(Player->HModelControlBlock)
    {
        EnhancedLegs.pLegsDisplay->HModelControlBlock = CloneHModel(Player->HModelControlBlock);
        
        // Creature-specific joint hiding
        GL_HideCreatureJoints();
        
        // Apply creature-specific materials/colors
        GL_ApplyCreatureMaterials();
    }
    
    // Set flags
    EnhancedLegs.pLegsDisplay->ObFlags |= ObFlag_NoCollision;
    EnhancedLegs.pLegsDisplay->ObFlags |= ObFlag_Ghost;
    EnhancedLegs.pLegsDisplay->ObParent = Player;
    
    // Add to render list
    AddToActiveBlockList(EnhancedLegs.pLegsDisplay);
    
    EnhancedLegs.bActive = 1;
    
    #ifdef _DEBUG
    const char* creatureNames[] = {"Marine", "Alien", "Predator"};
    textprint("%s legs created (%d legs)\n",
        creatureNames[EnhancedLegs.iCreatureType],
        EnhancedLegs.pCurrentConfig->iLegCount);
    #endif
}

/* ================================================================
   Hide Creature-Specific Joints
   ================================================================ */
void GL_HideCreatureJoints(void)
{
    if(!EnhancedLegs.pLegsDisplay || !EnhancedLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    HMODELCONTROLLER* pHModel = EnhancedLegs.pLegsDisplay->HModelControlBlock;
    
    // Hide joints from configuration
    for(int i = 0; i < EnhancedLegs.pCurrentConfig->iNumJointsToHide; i++)
    {
        const char* jointName = EnhancedLegs.pCurrentConfig->pJointsToHide[i];
        if(!jointName) break;
        
        SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, jointName);
        if(pSection)
        {
            pSection->flags |= SECTION_HIDDEN;
        }
    }
    
    // Creature-specific special handling
    switch(EnhancedLegs.iCreatureType)
    {
        case I_Alien:
            GL_HideAlienSpecificParts(pHModel);
            break;
            
        case I_Predator:
            GL_HidePredatorSpecificParts(pHModel);
            break;
    }
}

/* ================================================================
   Alien-Specific Part Hiding
   ================================================================ */
void GL_HideAlienSpecificParts(HMODELCONTROLLER* pHModel)
{
    // Alien has special parts to hide
    const char* alienSpecialParts[] = {
        "inner_jaw", "tongue",
        "acid_sac", "acid_drool",
        "ovipositor", "egg_sac",
        NULL
    };
    
    for(int i = 0; alienSpecialParts[i]; i++)
    {
        SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, alienSpecialParts[i]);
        if(pSection)
        {
            pSection->flags |= SECTION_HIDDEN;
        }
    }
    
    // Make legs slightly transparent for wall-crawling effect
    SECTION_DATA* pLegSection = GetThisSectionData(pHModel->section_data, "leg_front_left_upper");
    if(pLegSection && pLegSection->material)
    {
        // Adjust material transparency
        pLegSection->material->alpha = 0.9f;
    }
}

/* ================================================================
   Predator-Specific Part Hiding
   ================================================================ */
void GL_HidePredatorSpecificParts(HMODELCONTROLLER* pHModel)
{
    // Predator has equipment to hide
    const char* predatorEquipment[] = {
        "bio_mask", "laser_sight",
        "cloak_generator", "energy_cells",
        "medicomp", "self_destruct",
        NULL
    };
    
    for(int i = 0; predatorEquipment[i]; i++)
    {
        SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, predatorEquipment[i]);
        if(pSection)
        {
            pSection->flags |= SECTION_HIDDEN;
        }
    }
    
    // Make Predator legs more metallic
    SECTION_DATA* pLegSection = GetThisSectionData(pHModel->section_data, "leg_left_upper");
    if(pLegSection && pLegSection->material)
    {
        // Adjust material for Predator look
        pLegSection->material->shininess = 0.8f; // More shiny
    }
}

/* ================================================================
   Apply Creature-Specific Materials
   ================================================================ */
void GL_ApplyCreatureMaterials(void)
{
    if(!EnhancedLegs.pLegsDisplay || !EnhancedLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    HMODELCONTROLLER* pHModel = EnhancedLegs.pLegsDisplay->HModelControlBlock;
    
    // Apply creature-specific material adjustments
    switch(EnhancedLegs.iCreatureType)
    {
        case I_Marine:
            // Marine: standard military boots/legs
            GL_ApplyMarineMaterials(pHModel);
            break;
            
        case I_Alien:
            // Alien: chitinous, organic look
            GL_ApplyAlienMaterials(pHModel);
            break;
            
        case I_Predator:
            // Predator: metallic, high-tech
            GL_ApplyPredatorMaterials(pHModel);
            break;
    }
}

/* ================================================================
   Creature-Specific Material Functions
   ================================================================ */
void GL_ApplyMarineMaterials(HMODELCONTROLLER* pHModel)
{
    // Marine legs: military boots, camo pants
    SECTION_DATA* pLeg = GetThisSectionData(pHModel->section_data, "leg_left_upper");
    if(pLeg && pLeg->material)
    {
        // Camo pattern for pants
        pLeg->material->texture = "textures/marine_pants.cel";
        pLeg->material->reflectivity = 0.1f; // Matte
    }
    
    SECTION_DATA* pBoot = GetThisSectionData(pHModel->section_data, "leg_left_foot");
    if(pBoot && pBoot->material)
    {
        // Boots texture
        pBoot->material->texture = "textures/marine_boots.cel";
        pBoot->material->bumpiness = 0.3f; // Slight bump for boot tread
    }
}

void GL_ApplyAlienMaterials(HMODELCONTROLLER* pHModel)
{
    // Alien legs: chitinous, slimy
    for(int i = 0; i < 6; i++)
    {
        char legName[32];
        sprintf(legName, "leg_%d_upper", i);
        
        SECTION_DATA* pLeg = GetThisSectionData(pHModel->section_data, legName);
        if(pLeg && pLeg->material)
        {
            // Alien chitin texture
            pLeg->material->texture = "textures/alien_chitin.cel";
            pLeg->material->shininess = 0.4f; // Slightly shiny
            pLeg->material->specular = 0.2f;  // Wet look
        }
    }
}

void GL_ApplyPredatorMaterials(HMODELCONTROLLER* pHModel)
{
    // Predator legs: metallic, high-tech
    SECTION_DATA* pLeg = GetThisSectionData(pHModel->section_data, "leg_left_upper");
    if(pLeg && pLeg->material)
    {
        // Predator armor texture
        pLeg->material->texture = "textures/predator_armor.cel";
        pLeg->material->shininess = 0.9f;     // Very shiny
        pLeg->material->reflectivity = 0.3f;  // Slightly reflective
        pLeg->material->specular = 0.5f;      // Strong highlights
    }
    
    SECTION_DATA* pBoot = GetThisSectionData(pHModel->section_data, "boot_left");
    if(pBoot && pBoot->material)
    {
        // High-tech boots
        pBoot->material->texture = "textures/predator_boots.cel";
        pBoot->material->emissive = 0.1f;     // Slight glow
    }
}

/* ================================================================
   Creature-Specific Leg Animation
   ================================================================ */
void GL_AnimateCreatureLegs(void)
{
    if(!EnhancedLegs.bActive || !EnhancedLegs.pCurrentConfig)
        return;
    
    PLAYER_STATUS* pPlayerStatus = (PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
    DYNAMICSBLOCK* pDyn = Player->ObStrategyBlock->DynPtr;
    
    if(!pDyn)
        return;
    
    // Get movement speed
    int iSpeed = Approximate3dMagnitude(&pDyn->LinVelocity);
    
    // Creature-specific animation
    switch(EnhancedLegs.iCreatureType)
    {
        case I_Marine:
            GL_AnimateMarineLegs(iSpeed, pPlayerStatus);
            break;
            
        case I_Alien:
            GL_AnimateAlienLegs(iSpeed, pPlayerStatus);
            break;
            
        case I_Predator:
            GL_AnimatePredatorLegs(iSpeed, pPlayerStatus);
            break;
    }
    
    // Update leg positions on model
    GL_UpdateLegPositions();
}

/* ================================================================
   Marine Leg Animation (Human Biped)
   ================================================================ */
void GL_AnimateMarineLegs(int iSpeed, PLAYER_STATUS* pStatus)
{
    // Marine has standard human bipedal gait
    
    // Update leg phases based on speed
    int iPhaseIncrement = MUL_FIXED(iSpeed, EnhancedLegs.pCurrentConfig->iSwingMultiplier);
    
    // Left leg (phase 0)
    EnhancedLegs.iLegPhases[0] = (EnhancedLegs.iLegPhases[0] + iPhaseIncrement) & 65535;
    
    // Right leg (180 degrees out of phase)
    EnhancedLegs.iLegPhases[1] = (EnhancedLegs.iLegPhases[0] + 32768) & 65535;
    
    // Calculate swing amounts
    for(int i = 0; i < 2; i++)
    {
        int iPhase = EnhancedLegs.iLegPhases[i];
        int iSwing = GetSin(MUL_FIXED(iPhase, 32768)); // -1 to 1
        
        // Adjust swing based on crouching
        if(pStatus->ShapeState == PMph_Crouching)
        {
            iSwing = MUL_FIXED(iSwing, ONE_FIXED / 2); // Smaller steps when crouched
        }
        
        EnhancedLegs.iSwingIntensity[i] = MUL_FIXED(iSwing, iSpeed / 5000);
        
        // Calculate leg position
        EnhancedLegs.vTargetLegPositions[i].vx = (i == 0) ? 
            EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vx :
            EnhancedLegs.pCurrentConfig->vLegOffsetRight.vx;
        
        EnhancedLegs.vTargetLegPositions[i].vy = EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vy;
        EnhancedLegs.vTargetLegPositions[i].vz = MUL_FIXED(iSwing, 20 * ONE_FIXED);
    }
}

/* ================================================================
   Alien Leg Animation (6-Leg Tripod Gait)
   ================================================================ */
void GL_AnimateAlienLegs(int iSpeed, PLAYER_STATUS* pStatus)
{
    // Alien uses tripod gait: 3 legs move, 3 legs planted
    
    int iPhaseIncrement = MUL_FIXED(iSpeed, EnhancedLegs.pCurrentConfig->iSwingMultiplier);
    
    // Update all leg phases
    for(int i = 0; i < 6; i++)
    {
        EnhancedLegs.iLegPhases[i] = (EnhancedLegs.iLegPhases[i] + iPhaseIncrement) & 65535;
        
        // Tripod groups:
        // Group 1 (move together): legs 0, 3, 4
        // Group 2 (move together): legs 1, 2, 5
        int iGroupOffset = ((i == 0 || i == 3 || i == 4) ? 0 : 32768);
        int iPhase = (EnhancedLegs.iLegPhases[i] + iGroupOffset) & 65535;
        
        int iSwing = GetSin(MUL_FIXED(iPhase, 32768));
        
        // Alien has lower, more subtle leg movement
        EnhancedLegs.iSwingIntensity[i] = MUL_FIXED(iSwing, iSpeed / 8000);
        
        // Calculate leg position based on which leg it is
        int iLegPair = i / 2; // 0=front, 1=middle, 2=back
        int iLegSide = i % 2; // 0=left, 1=right
        
        EnhancedLegs.vTargetLegPositions[i].vx = (iLegSide == 0) ? 
            -EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vx :
            EnhancedLegs.pCurrentConfig->vLegOffsetRight.vx;
        
        EnhancedLegs.vTargetLegPositions[i].vy = EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vy - 
                                               (iLegPair * 40 * ONE_FIXED);
        
        // Alien legs lift less, more forward motion
        EnhancedLegs.vTargetLegPositions[i].vz = MUL_FIXED(iSwing, 15 * ONE_FIXED);
        
        // Extra forward motion for alien stride
        int iForward = GetSin(MUL_FIXED(iPhase, 16384));
        EnhancedLegs.vTargetLegPositions[i].vy += MUL_FIXED(iForward, 10 * ONE_FIXED);
    }
    
    // Wall-crawling adaptation
    if(pStatus->ShapeState == PMph_Crouching && EnhancedLegs.pCurrentConfig->bCanCrawlWalls)
    {
        GL_AdaptAlienToWalls();
    }
}

/* ================================================================
   Alien Wall-Crawling Adaptation
   ================================================================ */
void GL_AdaptAlienToWalls(void)
{
    // Check if alien is on a wall/ceiling
    DYNAMICSBLOCK* pDyn = Player->ObStrategyBlock->DynPtr;
    
    if(pDyn->UseStandardGravity == 0) // Alien wall-crawling mode
    {
        EnhancedLegs.bOnWall = 1;
        
        // Adjust leg positions for wall orientation
        VECTORCH vGravityDir = pDyn->GravityDirection;
        
        for(int i = 0; i < 6; i++)
        {
            // Rotate leg positions to match wall surface
            RotateVectorToSurface(&EnhancedLegs.vTargetLegPositions[i], &vGravityDir);
            
            // Legs grip into wall slightly
            EnhancedLegs.vTargetLegPositions[i].vz -= 5 * ONE_FIXED;
        }
    }
    else
    {
        EnhancedLegs.bOnWall = 0;
    }
}

/* ================================================================
   Predator Leg Animation (Powerful Stride)
   ================================================================ */
void GL_AnimatePredatorLegs(int iSpeed, PLAYER_STATUS* pStatus)
{
    // Predator has powerful, deliberate strides
    
    int iPhaseIncrement = MUL_FIXED(iSpeed, EnhancedLegs.pCurrentConfig->iSwingMultiplier);
    
    // Update leg phases
    EnhancedLegs.iLegPhases[0] = (EnhancedLegs.iLegPhases[0] + iPhaseIncrement) & 65535;
    EnhancedLegs.iLegPhases[1] = (EnhancedLegs.iLegPhases[0] + 49152) & 65535; // 270° offset
    
    for(int i = 0; i < 2; i++)
    {
        int iPhase = EnhancedLegs.iLegPhases[i];
        
        // Predator has higher knee lift
        int iSwing;
        if(iPhase < 16384) // First quarter: fast lift
        {
            iSwing = DIV_FIXED(iPhase * 4, 16384);
        }
        else if(iPhase < 49152) // Middle half: hold high
        {
            iSwing = ONE_FIXED;
        }
        else // Last quarter: fast drop
        {
            iSwing = DIV_FIXED((65536 - iPhase) * 4, 16384);
        }
        
        EnhancedLegs.iSwingIntensity[i] = MUL_FIXED(iSwing, iSpeed / 4000);
        
        // Calculate leg position
        EnhancedLegs.vTargetLegPositions[i].vx = (i == 0) ? 
            EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vx :
            EnhancedLegs.pCurrentConfig->vLegOffsetRight.vx;
        
        EnhancedLegs.vTargetLegPositions[i].vy = EnhancedLegs.pCurrentConfig->vLegOffsetLeft.vy;
        
        // Higher lift for predator
        EnhancedLegs.vTargetLegPositions[i].vz = MUL_FIXED(iSwing, 30 * ONE_FIXED);
    }
    
    // Cloaking effect on legs
    if(pStatus->ProcWalk.cloakOn)
    {
        GL_ApplyPredatorCloakEffect();
    }
}

/* ================================================================
   Predator Cloak Effect on Legs
   ================================================================ */
void GL_ApplyPredatorCloakEffect(void)
{
    PLAYER_STATUS* pStatus = (PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
    
    if(pStatus->ProcWalk.CloakingEffectiveness > 0)
    {
        // Make legs partially transparent when cloaked
        if(EnhancedLegs.pLegsDisplay && EnhancedLegs.pLegsDisplay->HModelControlBlock)
        {
            HMODELCONTROLLER* pHModel = EnhancedLegs.pLegsDisplay->HModelControlBlock;
            
            // Find leg materials and adjust alpha
            for(int i = 0; i < EnhancedLegs.pCurrentConfig->iNumLegJoints; i++)
            {
                const char* jointName = EnhancedLegs.pCurrentConfig->pLegJoints[i];
                if(!jointName) break;
                
                SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, jointName);
                if(pSection && pSection->material)
                {
                    // Reduce alpha based on cloak effectiveness
                    float fAlpha = 1.0f - (pStatus->ProcWalk.CloakingEffectiveness / 65536.0f);
                    pSection->material->alpha = fAlpha;
                }
            }
        }
    }
}

/* ================================================================
   Update Leg Positions on Model
   ================================================================ */
void GL_UpdateLegPositions(void)
{
    if(!EnhancedLegs.bActive || !EnhancedLegs.pLegsDisplay || 
       !EnhancedLegs.pLegsDisplay->HModelControlBlock)
        return;
    
    HMODELCONTROLLER* pHModel = EnhancedLegs.pLegsDisplay->HModelControlBlock;
    
    // Smoothly interpolate leg positions
    for(int i = 0; i < EnhancedLegs.pCurrentConfig->iLegCount; i++)
    {
        // Interpolate toward target position
        VECTORCH vDelta = SubtractVectors(&EnhancedLegs.vTargetLegPositions[i],
                                         &EnhancedLegs.vLegPositions[i]);
        
        vDelta = MultiplyVector(&vDelta, ONE_FIXED / 10); // 10% per frame
        EnhancedLegs.vLegPositions[i] = AddVectors(&EnhancedLegs.vLegPositions[i], &vDelta);
        
        // Apply to leg joint if found
        if(i < EnhancedLegs.pCurrentConfig->iNumLegJoints)
        {
            const char* jointName = EnhancedLegs.pCurrentConfig->pLegJoints[i];
            if(jointName)
            {
                SECTION_DATA* pSection = GetThisSectionData(pHModel->section_data, jointName);
                if(pSection)
                {
                    // Apply position offset to joint
                    pSection->World_Offset = EnhancedLegs.vLegPositions[i];
                }
            }
        }
    }
}

/* ================================================================
   Console Commands for Creature Testing
   ================================================================ */
void GL_CreatureDebug(void)
{
    const char* creatureNames[] = {"Marine", "Alien", "Predator"};
    
    textprint("=== Creature Legs Debug ===\n");
    textprint("Current Creature: %s\n", creatureNames[EnhancedLegs.iCreatureType]);
    textprint("Leg Count: %d\n", EnhancedLegs.pCurrentConfig->iLegCount);
    textprint("Active: %d\n", EnhancedLegs.bActive);
    textprint("On Wall: %d\n", EnhancedLegs.bOnWall);
    
    textprint("\nLeg Phases:\n");
    for(int i = 0; i < EnhancedLegs.pCurrentConfig->iLegCount; i++)
    {
        textprint("  Leg %d: Phase %d, Swing %d\n", 
            i, EnhancedLegs.iLegPhases[i] >> 8, // Scale for readability
            EnhancedLegs.iSwingIntensity[i] >> 8);
    }
    
    textprint("\nTarget Positions:\n");
    for(int i = 0; i < EnhancedLegs.pCurrentConfig->iLegCount; i++)
    {
        textprint("  Leg %d: (%d, %d, %d)\n", i,
            EnhancedLegs.vTargetLegPositions[i].vx >> 16,
            EnhancedLegs.vTargetLegPositions[i].vy >> 16,
            EnhancedLegs.vTargetLegPositions[i].vz >> 16);
    }
}

void GL_TestCreatureSwitch(void)
{
    // Cycle through creatures for testing
    static int testCreature = 0;
    
    testCreature = (testCreature + 1) % 3;
    
    // Simulate switching creatures (in real game, this would happen naturally)
    switch(testCreature)
    {
        case 0:
            AvP.PlayerType = I_Marine;
            textprint("Testing: Marine legs\n");
            break;
        case 1:
            AvP.PlayerType = I_Alien;
            textprint("Testing: Alien legs (6 legs)\n");
            break;
        case 2:
            AvP.PlayerType = I_Predator;
            textprint("Testing: Predator legs\n");
            break;
    }
    
    // Reinitialize with new creature
    GL_DestroyCreatureLegs();
    GL_InitCreatureLegs();
}